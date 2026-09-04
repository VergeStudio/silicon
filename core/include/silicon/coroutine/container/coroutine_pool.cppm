module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <expected>
#include <system_error>

export module silicon.coroutine:coroutine_pool;

import silicon.scheduler;
import silicon.scheduler.task;
import silicon.coroutine.error;
import :channel;
import :event;
import :mutex;

export namespace silicon::coroutine {

/**
 * @brief 复用 worker 协程池（固定 N 个长生命周期 worker + 工作队列）。
 *
 * 提交的任务仍分配自身协程帧；池的核心收益是 **复用 N 个 worker 帧** 与
 * **并发上限 = N**：超额任务在 channel 中排队，由空闲 worker 取出运行。
 *
 * `coroutine_pool` 本身实现 `silicon::scheduler::concepts::executor`，可直接作为执行器接入
 * `task_container` / `latch` 等现有设施——其 `spawn_*` 走池以限制并发，
 * 而 `schedule` / `yield` / `resume` 委托给底层执行器。
 *
 * @tparam Executor 底层执行器类型（须满足 `silicon::scheduler::concepts::executor`，如
 *                  `silicon::scheduler::thread_pool`）。
 */
template<silicon::scheduler::concepts::executor Executor>
class coroutine_pool {
  private:
    explicit coroutine_pool(std::shared_ptr<Executor> executor, std::size_t pool_size)
        : m_p(std::make_unique<impl>(pool_size)) {
        m_p->m_executor = std::move(executor);
        // 在底层执行器上拉起 N 个常驻 worker 协程（"池"本体）。
        for(std::size_t i = 0; i < pool_size; ++i) {
            m_p->m_workers_active.fetch_add(1, std::memory_order::release);
            (void)m_p->m_executor->spawn_detached(worker());
        }
    }

  public:
    /**
     * @brief 构造可失败工厂：校验 executor 非空、pool_size > 0，成功后返回
     *        std::unique_ptr<coroutine_pool>。失败时返回
     *        std::unexpected(coroutine_error::kNullExecutor / kInvalidPoolSize)。
     *        调用方应检查返回值，而非依赖异常。
     *
     * @param executor 底层执行器（任务经其线程池运行 worker）。不可为 nullptr。
     * @param pool_size worker 协程数（并发上限）。须 > 0。
     */
    static std::expected<std::unique_ptr<coroutine_pool<Executor>>, std::error_code> create(std::shared_ptr<Executor> executor, std::size_t pool_size) {
        if(executor == nullptr) {
            return std::unexpected(make_error_code(coroutine_error::kNullExecutor));
        }
        if(pool_size == 0) {
            return std::unexpected(make_error_code(coroutine_error::kInvalidPoolSize));
        }
        // create() 为成员函数，可访问私有构造；std::make_unique 无 friend 权限故用 new。
        return std::unique_ptr<coroutine_pool<Executor>>(
            new coroutine_pool<Executor>(std::move(executor), pool_size));
    }

    coroutine_pool(const coroutine_pool&)                    = delete;
    coroutine_pool(coroutine_pool&&)                         = delete;
    coroutine_pool& operator=(const coroutine_pool&) = delete;
    coroutine_pool& operator=(coroutine_pool&&) = delete;

    ~coroutine_pool() {
        shutdown();
        // 自旋（短 sleep）至在途任务清空且 worker 全部退出，避免悬挂与 m_p 提前释放后的 use-after-free
        // 跑竞态（同 task_container 析构）。
        // 注意：不要从运行在同一底层执行器上的协程内析构本对象，否则可能死锁。
        while(!empty() || m_p->m_workers_active.load(std::memory_order::acquire) > 0
              || !m_p->m_close_done.load(std::memory_order::acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }

    /**
     * @brief 便捷提交：等价于 spawn_detached。返回是否成功入队。
     */
    bool dispatch(silicon::scheduler::task<void>&& work) {
        return spawn_detached(std::move(work));
    }

    /**
     * @brief silicon::scheduler::concepts::executor 要求：提交任务到工作队列，由 worker 取出运行。
     * @return 是否成功入队（已 shutdown 或底层执行器拒绝时返回 false）。
     */
    bool spawn_detached(silicon::scheduler::task<void>&& work) {
        if(m_p->m_stopped.load(std::memory_order::acquire)) {
            return false;
        }
        // 同步计入在途与待发计数：保证析构自旋能在 sender 协程实际运行前就感知本任务，
        // 也保证 async_close 会等待本 sender 真正入队后才关闭通道（不丢任务）。
        m_p->m_inflight.fetch_add(1, std::memory_order::relaxed);
        m_p->m_pending_sends.fetch_add(1, std::memory_order::relaxed);
        auto ok = m_p->m_executor->spawn_detached(sender(std::move(work)));
        if(!ok) {
            // 底层执行器拒绝入队，回退两路计数。
            m_p->m_inflight.fetch_sub(1, std::memory_order::release);
            m_p->m_pending_sends.fetch_sub(1, std::memory_order::release);
        }
        return ok;
    }

    /**
     * @brief silicon::scheduler::concepts::executor 要求：提交任务，返回可 co_await 的 join 任务。
     * @return 一个在用户任务完成后置位的协程；入队失败时立即可完成（不悬挂）。
     */
    silicon::scheduler::task<void> spawn_joinable(silicon::scheduler::task<void>&& work) {
        auto e = std::make_shared<silicon::coroutine::event>();
        if(!spawn_detached(make_wrapper(this, e, std::move(work)))) {
            e->set(); // 入队失败则立即放行 join，避免悬挂。
        }
        // MSVC module-boundary workaround: a lambda coroutine capturing a
        // shared_ptr<event> inside this exported template member gets a corrupt
        // coroutine frame -- co_await *e dereferences a dangling event
        // (event::is_set() observed this == code-section address -> SIGSEGV).
        // Use a named coroutine function so the capture lands in the frame.
        return make_join_task(e);
    }

    /// Waits until the event is set. Named coroutine (not a lambda) to dodge
    /// the MSVC lambda-capture-in-exported-template bug.
    static silicon::scheduler::task<void> make_join_task(std::shared_ptr<silicon::coroutine::event> e) {
        co_await *e;
    }
    /// Runs the user task, then signals the event. Named coroutine (same MSVC
    /// workaround as make_join_task: lambda captures in exported templates get
    /// corrupt frames).
    static silicon::scheduler::task<void> make_wrapper(coroutine_pool *self, std::shared_ptr<silicon::coroutine::event> e,
                             silicon::scheduler::task<void> w) {
        try {
            co_await std::move(w);
        } catch(...) {
            self->capture_error();
        }
        e->set();
    }

    /**
     * @return 当前在途（排队 + 运行）任务数。
     */
    [[nodiscard]] std::size_t size() const {
        return m_p->m_inflight.load(std::memory_order::acquire);
    }

    /**
     * @return 是否无在途任务。
     */
    [[nodiscard]] bool empty() const { return size() == 0; }

    /**
     * @brief co_await 直到所有在途任务完成。worker 继续常驻，可继续 dispatch。
     */
    silicon::scheduler::task<void> join() {
        while(!empty()) {
            co_await m_p->m_executor->yield();
        }
    }

    /**
     * @brief 关闭工作队列。已入队任务仍会被 worker 排空，随后 worker 退出。幂等。
     *
     * 关闭通过 async_close 协程异步完成：它先等待所有待发 sender 真正入队，
     * 再关闭通道（drain 语义），从而避免在 dispatch 后立即析构时丢失任务。
     */
    void shutdown() {
        if(m_p->m_stopped.exchange(true, std::memory_order::acq_rel)) {
            return;
        }
        (void)m_p->m_executor->spawn_detached(async_close());
    }

    /// @brief silicon::scheduler::concepts::executor 要求：调度委托给底层执行器。
    auto schedule() { return m_p->m_executor->schedule(); }
    /// @brief silicon::scheduler::concepts::executor 要求：让出委托给底层执行器。
    auto yield() { return m_p->m_executor->yield(); }
    /// @brief silicon::scheduler::concepts::executor 要求：恢复句柄委托给底层执行器。
    bool resume(std::coroutine_handle<> handle) { return m_p->m_executor->resume(handle); }

    /**
     * @brief 最后捕获的未处理异常（worker 内任务抛错时记录，不重抛以免击垮 worker）。
     */
    [[nodiscard]] std::exception_ptr last_error() const { return m_p->m_last_error; }

  private:
    void capture_error() {
        if(!m_p->m_last_error) {
            m_p->m_last_error = std::current_exception();
        }
    }

    // 常驻 worker：循环取任务 -> 复用本协程帧运行 -> 循环；通道关闭且排空后退出。
    silicon::scheduler::task<void> worker() {
        while(true) {
            auto got = co_await m_p->m_channel.recv();
            if(!got.has_value()) {
                break; // 通道已关闭并排空
            }
            auto work = std::move(got).value();
            try {
                co_await std::move(work);
            } catch(...) {
                capture_error();
            }
            // 任务完成（无论成败）后释放一个在途名额。
            m_p->m_inflight.fetch_sub(1, std::memory_order::release);
        }
        // 退出前释放常驻 worker 名额，允许析构等待其完成退出。
        m_p->m_workers_active.fetch_sub(1, std::memory_order::release);
    }

    // 生产者协程：把任务送入通道（通道满时挂起，由消费者腾槽后唤醒）。
    silicon::scheduler::task<void> sender(silicon::scheduler::task<void> work) {
        auto result = co_await m_p->m_channel.send(std::move(work));
        if(result == channel_result::send::kClosed) {
            // 通道已关闭，任务未被接收入队，回退在途计数（worker 不会处理它）。
            m_p->m_inflight.fetch_sub(1, std::memory_order::release);
        }
        // 无论 kSent / kClosed，sender 至此结束，减少待发计数。
        m_p->m_pending_sends.fetch_sub(1, std::memory_order::release);
    }

    // 异步关闭：先等待所有待发 sender 真正入队（期间通道保持开启，不丢任务），
    // 再关闭通道（drain 语义），worker 排空后退出。
    silicon::scheduler::task<void> async_close() {
        while(m_p->m_pending_sends.load(std::memory_order::acquire) > 0) {
            co_await m_p->m_executor->yield();
        }
        co_await m_p->m_channel.close();
        m_p->m_close_done.store(true, std::memory_order::release);
    }

    struct impl {
      public:
        explicit impl(std::size_t capacity)
            : m_channel{capacity} {}

        std::shared_ptr<Executor> m_executor{nullptr};
        channel<silicon::scheduler::task<void>> m_channel;
        std::atomic<std::size_t> m_inflight{0};
        std::atomic<std::size_t> m_pending_sends{0};
        std::atomic<std::size_t> m_workers_active{0};
        std::atomic<bool> m_close_done{false};
        std::atomic<bool> m_stopped{false};
        std::exception_ptr m_last_error{nullptr};
    };

    std::unique_ptr<impl> m_p;
};

} // namespace silicon::coroutine
