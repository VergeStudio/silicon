














module;

#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <memory>
#include <mutex>
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
#    include <io.h>
#else
#    include <cerrno>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

module silicon.scheduler;


#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_op;



using namespace silicon::scheduler;

namespace silicon::scheduler {




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




constexpr std::chrono::milliseconds kCompletionWorkerIdleTick{10};




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


    [[nodiscard]] bool start() noexcept;

    void stop_and_join() noexcept;


    [[nodiscard]] bool enqueue(io_op *op) noexcept;

    [[nodiscard]] io_op *take_all_completed() noexcept;

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

    std::atomic<io_op *> m_pending{nullptr};

    std::atomic<io_op *> m_completed{nullptr};

    std::atomic<bool> m_stop{false};
    std::thread m_worker;
    std::chrono::milliseconds m_idle_tick{kCompletionWorkerIdleTick};
#if defined(SILICON_PLATFORM_LINUX)


    silicon::scheduler::pipe_t m_wake_pipe{};
    bool m_wake_registered{false};
#endif
};

completion_engine::completion_engine(const io_scheduler::options &opts, io_notifier &notifier, void *sentinel)
    : m_notifier(notifier)
    , m_sentinel(sentinel) {
    if(opts.completion_policy == io_scheduler::io_completion_policy::disabled) { return; }

    m_ring = std::make_unique<io_ring>(opts.io_ring_cfg);
    if(!m_ring->is_valid()) { m_ring.reset(); return; }
    if(!m_ring->supports(io_ring::op::read) || !m_ring->supports(io_ring::op::write)) { m_ring.reset(); return; }

#if defined(SILICON_PLATFORM_LINUX)
    auto created = pipe_t::create();
    if(!created) {
        m_ring.reset();
        return;
    }
    m_wake_pipe = std::move(*created);


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
    silicon::scheduler::awaiter_list_push(m_pending, op);
    return true;
}

io_op * completion_engine::take_all_completed() noexcept {
    return silicon::scheduler::awaiter_list_pop_all(m_completed);
}

void completion_engine::drain_wake_pipe() noexcept {
#if defined(SILICON_PLATFORM_LINUX)
    if(!m_wake_pipe.is_valid()) { return; }
    char buffer[64]{};
    while(true) {
        const long n = m_wake_pipe.read(buffer, sizeof(buffer));
        if(n > 0) { continue; }

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
    while(io_op *op = silicon::scheduler::awaiter_list_pop(m_pending)) {
        if(!submit_op(op)) {

            m_ring->submit();
            if(!submit_op(op)) {
                op->complete_error(make_error_code(scheduler_error::kCompletionSubmitFailed));
                silicon::scheduler::awaiter_list_push(m_completed, op);
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

    silicon::scheduler::awaiter_list_push(m_completed, op);
    wake_driver();
}

void completion_engine::wake_driver() noexcept {
#if defined(SILICON_PLATFORM_LINUX)
    if(!m_wake_pipe.is_valid()) { return; }
    const char byte = 1;
    const long written = m_wake_pipe.write(&byte, sizeof(byte));
    if(written != static_cast<long>(sizeof(byte))) {

    }
#elif defined(SILICON_PLATFORM_WINDOWS)
    if(m_sentinel != nullptr) {


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

            while(auto extra = m_ring->peek_completion()) {
                handle_completion(*extra);
            }
        }
    }



    submit_pending();
    while(io_op *op = silicon::scheduler::awaiter_list_pop(m_pending)) {
        op->complete_error(make_error_code(scheduler_error::kShuttingDown));
        silicon::scheduler::awaiter_list_push(m_completed, op);
    }
    wake_driver();
}



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

}

#endif





silicon::scheduler::task<result<int64_t>> io_scheduler::read_at(
        fd_t fd, void *buffer, std::uint32_t length, std::uint64_t offset
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
    op.m_buffer = buffer;
    op.m_length = length;
    op.m_offset = offset;
    op.m_is_write = false;

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


    engine->drain_wake_pipe();

    io_op *ops = engine->take_all_completed();
    if(ops == nullptr) { return; }
    ops = silicon::scheduler::awaiter_list_reverse(ops);

    while(ops != nullptr) {
        io_op *next = ops->m_next;
        if(!ops->m_processed) {
            ops->m_processed = true;


            while(ops->m_awaiting_coroutine == nullptr) {
                std::atomic_thread_fence(std::memory_order::acquire);
            }
            m_p->m_handles_to_resume.emplace_back(ops->m_awaiting_coroutine);
        }
        ops = next;
    }
#else

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

}
