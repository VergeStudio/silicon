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

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_scheduler;
import :io_op;

using namespace silicon::scheduler;

namespace silicon::scheduler {

// 平台无关骨架：completion 引擎与 read_at / write_at 等公开接口。
// 平台差异经接缝下沉到 io_scheduler_completion_unix.cpp / _linux.cpp / _win.cpp
// （接缝声明见 io_scheduler.cppm 非导出区）。

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
    completion_wake m_wake{};
};

completion_engine::completion_engine(const io_scheduler::options &opts, io_notifier &notifier, void *sentinel)
    : m_notifier(notifier)
    , m_sentinel(sentinel) {
    if(opts.completion_policy == io_scheduler::io_completion_policy::disabled) { return; }

    m_ring = std::make_unique<io_ring>(opts.io_ring_cfg);
    if(!m_ring->is_valid()) { m_ring.reset(); return; }
    if(!m_ring->supports(io_ring::op::read) || !m_ring->supports(io_ring::op::write)) { m_ring.reset(); return; }

    if(!m_wake.setup(m_notifier, m_sentinel)) {
        m_ring.reset();
        return;
    }

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
    m_wake.teardown(m_notifier);
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
    m_wake.drain();
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
        op->complete_error(completion_result_to_error(completion.result));
    }

    silicon::scheduler::awaiter_list_push(m_completed, op);
    wake_driver();
}

void completion_engine::wake_driver() noexcept {
    m_wake.notify(m_notifier, m_sentinel);
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

silicon::scheduler::task<silicon::error::result<int64_t>> io_scheduler::read_at(
        int fd, void *buffer, std::uint32_t length, std::uint64_t offset
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

silicon::scheduler::task<silicon::error::result<int64_t>> io_scheduler::write_at(
        int fd, const void *buffer, std::uint32_t length, std::uint64_t offset
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
