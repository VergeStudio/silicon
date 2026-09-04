// io_ring 接入 io_scheduler 第二层的 completion I/O 引擎实现单元。
//
// 本单元与 readiness 引擎（io_notifier 的 epoll/kqueue/IOCP）互补：read_at /
// write_at 只服务带偏移的常规文件，由专用 completion worker 线程独占 io_ring
// （Linux=io_uring / Windows=I/O Ring）提交并收割；CQE 完成后经两条 MPSC
// 侵入式链表把 io_op 从「待提交」推进到「已完成」，再经唤醒通道（POSIX 内部
// completion pipe / Windows io_notifier::post）叫醒事件驱动线程，由
// io_scheduler::drain_ring_completions 逐个恢复挂起协程。
//
// 平台纪律与 io_ring_uring.cpp / io_ring_ioring.cpp 一致：仅当编译出后端的
// 平台（linux/windows 且 SILICON_FEATURE_IO_RING 已定义）才触碰 io_ring；
// 其余平台本文件退化为纯默认契约路径（kNotRegularFile / kNoCompletionBackend）。
// io_scheduler::impl 的两个 opaque 槽（m_completion_engine / m_completion_ptr）
// 使平台专属类型绝不出现在 io_scheduler.cppm。

module;

#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex> // std::call_once（惰性探测）
#include <new>
#include <system_error>
#include <thread>
#include <utility>

#include <silicon/common.h>

#if defined(SILICON_PLATFORM_WINDOWS)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <windows.h>
#    include <io.h> // ::_get_osfhandle
#else
#    include <cerrno>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

module silicon.scheduler;
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫。
#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_op;

// 与 io_scheduler.cpp 一致：沿用 coroutine 基础类型（fd_t / poll_op / pipe_t）
// 与调度原语的简化书写，using-directive 置于全局作用域（不参与模块导出）。
using namespace silicon::coroutine;

namespace silicon::scheduler {

// ---------------------------------------------------------------------------
// 文件类型预检（所有平台）：completion I/O v1 只路由常规文件。
// ---------------------------------------------------------------------------
static bool completion_file_is_regular(fd_t fd) {
#if defined(SILICON_PLATFORM_WINDOWS)
    if(fd < 0) { return false; }
    intptr_t os_handle = ::_get_osfhandle(fd);
    if(os_handle == -1) { return false; }
    return ::GetFileType(reinterpret_cast<HANDLE>(os_handle)) == FILE_TYPE_DISK;
#else
    if(fd < 0) { return false; }
    struct ::stat file_stat {};
    if(::fstat(fd, &file_stat) != 0) { return false; }
    return S_ISREG(file_stat.st_mode);
#endif
}

#if defined(SILICON_FEATURE_IO_RING) &&                                                                  \
        (defined(SILICON_PLATFORM_LINUX) || defined(SILICON_PLATFORM_WINDOWS))

namespace {

/// completion worker 的轮询节拍：worker 阻塞在 io_ring::wait_completion 上，
/// 超时后回到循环顶部检查停摆与待提交队列。v1 未接入 eventfd 直连快路径
/// （设计 R6/F4 记为 v1.1 优化），故首次提交与关闭延迟以本节拍为上限。
constexpr std::chrono::milliseconds kCompletionWorkerIdleTick{10};

/// completion 引擎 opaque（模块内部，不导出）。生命周期归 io_scheduler::impl
/// 的 m_completion_engine（void* 槽）；io_ring 只在本引擎的 worker 线程上触碰，
/// 因此 io_ring 方法无需线程安全改造（设计 Decision 1 的 Actor 模型）。
class completion_engine {
  public:
    completion_engine(const io_scheduler::options &opts, io_notifier &notifier, void *sentinel);
    ~completion_engine();

    completion_engine(const completion_engine &) = delete;
    completion_engine(completion_engine &&) = delete;
    completion_engine & operator=(const completion_engine &) = delete;
    completion_engine & operator=(completion_engine &&) = delete;

    [[nodiscard]] io_ring::backend backend() const noexcept { return m_backend; }
    [[nodiscard]] bool available() const noexcept { return m_backend != io_ring::backend::none; }

    /// 启动专用 completion worker 线程；失败返回 false（调用方应销毁引擎）。
    [[nodiscard]] bool start() noexcept;
    /// 置停 worker 并 join，随后注销唤醒 fd、关闭 io_ring。
    void stop_and_join() noexcept;

    /// 生产者把 io_op 推入「待提交」MPSC 队列（release）。
    [[nodiscard]] bool enqueue(io_op *op) noexcept;
    /// 驱动线程摘取整条「已完成」MPSC 队列（acquire）。
    [[nodiscard]] io_op *take_all_completed() noexcept;
    /// 驱动线程排空内部 completion pipe（POSIX）。
    void drain_wake_pipe() noexcept;

  private:
    void worker_main() noexcept;
    void submit_pending() noexcept;
    void handle_completion(const io_ring::completion &completion) noexcept;
    [[nodiscard]] bool submit_op(io_op *op) noexcept;
    void wake_driver() noexcept;

    io_notifier &m_notifier;
    void *m_sentinel{nullptr};
    io_ring::backend m_backend{io_ring::backend::none};
    std::unique_ptr<io_ring> m_ring;
    /// 待提交：生产者 push → worker pop_all/submit。
    std::atomic<io_op *> m_pending{nullptr};
    /// 已完成：worker push → 驱动 drain pop_all。
    std::atomic<io_op *> m_completed{nullptr};
    /// 停摆标志。
    std::atomic<bool> m_stop{false};
    std::thread m_worker;
    std::chrono::milliseconds m_idle_tick{kCompletionWorkerIdleTick};
#if defined(SILICON_PLATFORM_LINUX)
    /// 内部 completion pipe：worker 写 1 字节唤醒事件驱动线程（epoll 可轮询）。
    /// 严禁接 io_scheduler 的 CRT schedule pipe（Windows 唤醒同理走 post）。
    silicon::coroutine::pipe_t m_wake_pipe{};
    bool m_wake_registered{false};
#endif
};

completion_engine::completion_engine(const io_scheduler::options &opts, io_notifier &notifier, void *sentinel)
    : m_notifier(notifier)
    , m_sentinel(sentinel) {
    if(opts.completion_policy == io_scheduler::io_completion_policy::disabled) { return; }

    m_ring = std::make_unique<io_ring>(opts.io_ring);
    if(!m_ring->is_valid()) { m_ring.reset(); return; }
    if(!m_ring->supports(io_ring::op::read) || !m_ring->supports(io_ring::op::write)) { m_ring.reset(); return; }

#if defined(SILICON_PLATFORM_LINUX)
    auto created = pipe_t::create();
    if(!created) {
        m_ring.reset();
        return;
    }
    m_wake_pipe = std::move(*created);
    // 哨兵必须是 impl 内真实 poll_info（m_completion_ptr）：epoll 的 next_events
    // 会把 udata 解引用成 poll_info* 读取 m_cancel_trigger，不能传任意地址。
    if(!m_notifier.watch(m_wake_pipe.read_fd(), poll_op::read, m_sentinel, true, false)) {
        m_wake_pipe.close();
        m_ring.reset();
        return;
    }
    m_wake_registered = true;
#endif

    m_backend = m_ring->active_backend();
}

completion_engine::~completion_engine() {
    stop_and_join();
}

bool completion_engine::start() noexcept {
    if(!available()) { return false; }
    try {
        m_worker = std::thread(&completion_engine::worker_main, this);
    } catch(...) {
        return false;
    }
    return true;
}

void completion_engine::stop_and_join() noexcept {
    m_stop.store(true, std::memory_order::release);
    if(m_worker.joinable()) {
        m_worker.join();
    }
#if defined(SILICON_PLATFORM_LINUX)
    if(m_wake_registered) {
        m_notifier.unwatch(m_wake_pipe.read_fd(), poll_op::read);
        m_wake_registered = false;
    }
    m_wake_pipe.close();
#endif
    m_ring.reset();
}

bool completion_engine::enqueue(io_op *op) noexcept {
    if(!available() || op == nullptr) { return false; }
    silicon::coroutine::awaiter_list_push(m_pending, op);
    return true;
}

io_op * completion_engine::take_all_completed() noexcept {
    return silicon::coroutine::awaiter_list_pop_all(m_completed);
}

void completion_engine::drain_wake_pipe() noexcept {
#if defined(SILICON_PLATFORM_LINUX)
    if(!m_wake_pipe.is_valid()) { return; }
    char buffer[64]{};
    while(true) {
        const long n = m_wake_pipe.read(buffer, sizeof(buffer));
        if(n > 0) { continue; }
        // O_NONBLOCK：读到 EAGAIN 说明管道已清空。
        if(n < 0 && errno == EAGAIN) { break; }
        break;
    }
#endif
}

bool completion_engine::submit_op(io_op *op) noexcept {
    const auto user_data = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(op));
    if(op->m_is_write) {
        return m_ring->submit_write(op->m_fd, op->m_buffer, op->m_length, op->m_offset, user_data);
    }
    return m_ring->submit_read(op->m_fd, op->m_buffer, op->m_length, op->m_offset, user_data);
}

void completion_engine::submit_pending() noexcept {
    while(io_op *op = silicon::coroutine::awaiter_list_pop(m_pending)) {
        if(!submit_op(op)) {
            // SQ 已满：先 submit 腾出槽位再重试；仍失败按提交失败完结。
            m_ring->submit();
            if(!submit_op(op)) {
                op->complete_error(make_error_code(scheduler_error::kCompletionSubmitFailed));
                silicon::coroutine::awaiter_list_push(m_completed, op);
                wake_driver();
                continue;
            }
        }
        m_ring->submit();
    }
}

void completion_engine::handle_completion(const io_ring::completion &completion) noexcept {
    auto *op = reinterpret_cast<io_op *>(static_cast<std::uintptr_t>(completion.user_data));
    if(op == nullptr) { return; }

    // 错误解码（设计 F2 注释点）：成功（含 0 字节）为传输字节数；
    // Linux <0 = -errno → generic_category；Windows 失败回填 HRESULT 负值 →
    // system_category（可读性待实证，必要时换自定义 scheduler 子类别）。
    if(completion.result > 0) {
        op->complete(static_cast<std::int64_t>(completion.result));
    } else if(completion.result == 0) {
        op->complete(0);
    } else {
#if defined(SILICON_PLATFORM_LINUX)
        op->complete_error(std::error_code(-completion.result, std::generic_category()));
#elif defined(SILICON_PLATFORM_WINDOWS)
        op->complete_error(std::error_code(static_cast<int>(completion.result), std::system_category()));
#endif
    }

    silicon::coroutine::awaiter_list_push(m_completed, op);
    wake_driver();
}

void completion_engine::wake_driver() noexcept {
#if defined(SILICON_PLATFORM_LINUX)
    if(!m_wake_pipe.is_valid()) { return; }
    const char byte = 1;
    const long written = m_wake_pipe.write(&byte, sizeof(byte));
    if(written != static_cast<long>(sizeof(byte))) {
        // O_NONBLOCK 管道：EAGAIN 表示唤醒字节仍在管道内（事件未丢失），忽略。
    }
#elif defined(SILICON_PLATFORM_WINDOWS)
    if(m_sentinel != nullptr) {
        // Windows 事件循环唤醒禁接 CRT schedule pipe（既有 IOCP 等待语义无法
        // 被 pipe 写可靠打断），必须 PostQueuedCompletionStatus（设计 R4）。
        m_notifier.post(m_sentinel);
    }
#endif
}

void completion_engine::worker_main() noexcept {
    while(!m_stop.load(std::memory_order::acquire)) {
        submit_pending();
        auto completion = m_ring->wait_completion(m_idle_tick);
        if(completion.has_value()) {
            handle_completion(*completion);
            // 排空同批已就绪的完成项，减少唤醒驱动线程的次数。
            while(auto extra = m_ring->peek_completion()) {
                handle_completion(*extra);
            }
        }
    }

    // 停摆收尾：尽量把仍在待提交队列的操作送进内核；正常关闭路径下此时队列
    // 已空（shutdown 会等全部挂起任务完成），此处只做防御性完结。
    submit_pending();
    while(io_op *op = silicon::coroutine::awaiter_list_pop(m_pending)) {
        op->complete_error(make_error_code(scheduler_error::kShuttingDown));
        silicon::coroutine::awaiter_list_push(m_completed, op);
    }
    wake_driver();
}

/// 惰性一次性探测：构造 io_ring、内部唤醒通道并启动 worker。
/// 任一环节失败均返回 nullptr（消费方降级为 kNoCompletionBackend），不抛异常。
completion_engine * create_completion_engine(
        const io_scheduler::options &opts, io_notifier &notifier, void *sentinel
) {
    std::unique_ptr<completion_engine> engine;
    try {
        engine = std::make_unique<completion_engine>(opts, notifier, sentinel);
    } catch(...) {
        return nullptr;
    }
    if(!engine->available()) { return nullptr; }
    if(!engine->start()) { return nullptr; }
    return engine.release();
}

} // namespace

#endif // SILICON_FEATURE_IO_RING && (LINUX || WINDOWS)

// ---------------------------------------------------------------------------
// io_scheduler::read_at / write_at —— completion 路由 + 文件预检 + 惰性探测。
// ---------------------------------------------------------------------------

silicon::scheduler::task<result<int64_t>> io_scheduler::read_at(
        fd_t fd, void *buffer, std::uint32_t length, std::uint64_t offset
) {
    // 路由矩阵（设计 Decision 2）：API 形态定路由，而非运行期猜 fd 类型。
    // 文件预检先于后端可用性检查：socket/pipe 一律 kNotRegularFile。
    if(!completion_file_is_regular(fd)) {
        co_return std::unexpected(make_error_code(scheduler_error::kNotRegularFile));
    }

#if defined(SILICON_FEATURE_IO_RING) &&                                                                  \
        (defined(SILICON_PLATFORM_LINUX) || defined(SILICON_PLATFORM_WINDOWS))
    if(m_p->m_completion_engine == nullptr) {
        std::call_once(m_p->m_completion_once, [&]() {
            m_p->m_completion_engine =
                    create_completion_engine(m_p->m_opts, m_p->m_io_notifier, m_p->m_completion_ptr);
        });
    }
    auto *engine = static_cast<completion_engine *>(m_p->m_completion_engine);
    if(engine == nullptr || !engine->available()) {
        co_return std::unexpected(make_error_code(scheduler_error::kNoCompletionBackend));
    }
    if(m_p->m_shutdown_requested.load(std::memory_order::acquire)) {
        co_return std::unexpected(make_error_code(scheduler_error::kShuttingDown));
    }

    m_p->m_size.fetch_add(1, std::memory_order::release);

    io_op op{};
    op.m_fd = fd;
    op.m_buffer = buffer;
    op.m_length = length;
    op.m_offset = offset;
    op.m_is_write = false;

    if(!engine->enqueue(&op)) {
        m_p->m_size.fetch_sub(1, std::memory_order::release);
        co_return std::unexpected(make_error_code(scheduler_error::kCompletionSubmitFailed));
    }

    // 入队先于挂起（镜像 poll() 的 watch-before-co_await 模式）：驱动在
    // drain_ring_completions 中以 acquire fence 自旋等待 await_suspend 写入
    // m_awaiting_coroutine 后再恢复，避免恢复先于挂起。
    co_await op;
    co_return op.result();
#else
    (void)buffer;
    (void)length;
    (void)offset;
    // 未编译 completion 后端：契约性降级（不挂起、不阻塞）。
    co_return std::unexpected(make_error_code(scheduler_error::kNoCompletionBackend));
#endif
}

silicon::scheduler::task<result<int64_t>> io_scheduler::write_at(
        fd_t fd, const void *buffer, std::uint32_t length, std::uint64_t offset
) {
    if(!completion_file_is_regular(fd)) {
        co_return std::unexpected(make_error_code(scheduler_error::kNotRegularFile));
    }

#if defined(SILICON_FEATURE_IO_RING) &&                                                                  \
        (defined(SILICON_PLATFORM_LINUX) || defined(SILICON_PLATFORM_WINDOWS))
    if(m_p->m_completion_engine == nullptr) {
        std::call_once(m_p->m_completion_once, [&]() {
            m_p->m_completion_engine =
                    create_completion_engine(m_p->m_opts, m_p->m_io_notifier, m_p->m_completion_ptr);
        });
    }
    auto *engine = static_cast<completion_engine *>(m_p->m_completion_engine);
    if(engine == nullptr || !engine->available()) {
        co_return std::unexpected(make_error_code(scheduler_error::kNoCompletionBackend));
    }
    if(m_p->m_shutdown_requested.load(std::memory_order::acquire)) {
        co_return std::unexpected(make_error_code(scheduler_error::kShuttingDown));
    }

    m_p->m_size.fetch_add(1, std::memory_order::release);

    io_op op{};
    op.m_fd = fd;
    op.m_buffer = const_cast<void *>(buffer);
    op.m_length = length;
    op.m_offset = offset;
    op.m_is_write = true;

    if(!engine->enqueue(&op)) {
        m_p->m_size.fetch_sub(1, std::memory_order::release);
        co_return std::unexpected(make_error_code(scheduler_error::kCompletionSubmitFailed));
    }

    co_await op;
    co_return op.result();
#else
    (void)buffer;
    (void)length;
    (void)offset;
    co_return std::unexpected(make_error_code(scheduler_error::kNoCompletionBackend));
#endif
}

// ---------------------------------------------------------------------------
// completion_backend / drain_ring_completions / destroy_completion_engine。
// ---------------------------------------------------------------------------

auto io_scheduler::completion_backend() const noexcept -> io_ring::backend {
    if(m_p == nullptr || m_p->m_completion_engine == nullptr) {
        return io_ring::backend::none;
    }
#if defined(SILICON_FEATURE_IO_RING) &&                                                                  \
        (defined(SILICON_PLATFORM_LINUX) || defined(SILICON_PLATFORM_WINDOWS))
    return static_cast<completion_engine *>(m_p->m_completion_engine)->backend();
#else
    return io_ring::backend::none;
#endif
}

void io_scheduler::drain_ring_completions() {
#if defined(SILICON_FEATURE_IO_RING) &&                                                                  \
        (defined(SILICON_PLATFORM_LINUX) || defined(SILICON_PLATFORM_WINDOWS))
    if(m_p == nullptr || m_p->m_completion_engine == nullptr) { return; }

    auto *engine = static_cast<completion_engine *>(m_p->m_completion_engine);
    // 先清空唤醒字节再摘完成队列：worker 先 push 完成项后写字节，两条路径的
    // 顺序保证不会丢唤醒（详细推理见 io_scheduler_completion.cpp 头注释）。
    engine->drain_wake_pipe();

    io_op *ops = engine->take_all_completed();
    if(ops == nullptr) { return; }
    ops = silicon::coroutine::awaiter_list_reverse(ops);

    while(ops != nullptr) {
        io_op *next = ops->m_next;
        if(!ops->m_processed) {
            ops->m_processed = true;
            // 生产者入队先于挂起：等待其 await_suspend 写入句柄（与
            // process_event_execute 的同款自旋，正常情况下已可见）。
            while(ops->m_awaiting_coroutine == nullptr) {
                std::atomic_thread_fence(std::memory_order::acquire);
            }
            m_p->m_handles_to_resume.emplace_back(ops->m_awaiting_coroutine);
        }
        ops = next;
    }
#else
    // 无 completion 后端：m_completion_ptr 哨兵永无事件，本方法不会被执行。
#endif
}

void io_scheduler::destroy_completion_engine() {
#if defined(SILICON_FEATURE_IO_RING) &&                                                                  \
        (defined(SILICON_PLATFORM_LINUX) || defined(SILICON_PLATFORM_WINDOWS))
    if(m_p == nullptr || m_p->m_completion_engine == nullptr) { return; }

    auto *engine = static_cast<completion_engine *>(m_p->m_completion_engine);
    engine->stop_and_join();
    delete engine;
    m_p->m_completion_engine = nullptr;
#else
    if(m_p != nullptr) {
        m_p->m_completion_engine = nullptr;
    }
#endif
}

} // namespace silicon::scheduler
