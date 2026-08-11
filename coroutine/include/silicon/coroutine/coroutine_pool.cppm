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

export module silicon.coroutine:coroutine_pool;

import silicon.scheduler;
import silicon.scheduler.task;
import :channel;
import :event;

export namespace silicon::coroutine {

/**
 * @brief 复用 worker 协程池（固定 N 个长生命周期 worker + 工作队列）。
 *
 * 提交的任务仍分配自身协程帧；池的核心收益是 **复用 N 个 worker 帧** 与
 * **并发上限 = N**：超额任务在 channel 中排队，由空闲 worker 取出运行。
 *
 * `coroutine_pool` 本身实现 `concepts::executor`，可直接作为执行器接入
 * `task_container` / `latch` 等现有设施——其 `spawn_*` 走池以限制并发，
 * 而 `schedule` / `yield` / `resume` 委托给底层执行器。
 *
 * @tparam Executor 底层执行器类型（须满足 `concepts::executor`，如
 *                  `silicon::scheduler::thread_pool`）。
 */
template<concepts::executor Executor>
class coroutine_pool {
  public:
    /**
     * @param executor 底层执行器（任务经其线程池运行 worker）。不可为 nullptr。
     * @param pool_size worker 协程数（并发上限）。须 > 0。
     */
    explicit coroutine_pool(std::shared_ptr<Executor> executor, std::size_t pool_size)
        : m_p(std::make_unique<Impl>(pool_size)) {
        m_p->m_executor = std::move(executor);
        if(m_p->m_executor == nullptr) {
            throw std::runtime_error{"coroutine_pool cannot have a nullptr executor"};
        }
        if(pool_size == 0) {
            throw std::runtime_error{"coroutine_pool requires a pool_size > 0"};
        }
        // 在底层执行器上拉起 N 个常驻 worker 协程（"池"本体）。
        for(std::size_t i = 0; i < pool_size; ++i) {
            (void)m_p->m_executor->spawn_detached(worker());
        }
    }

    coroutine_pool(const coroutine_pool&)                    = delete;
    coroutine_pool(coroutine_pool&&)                         = delete;
    auto operator=(const coroutine_pool&) -> coroutine_pool& = delete;
    auto operator=(coroutine_pool&&) -> coroutine_pool&      = delete;

    ~coroutine_pool() {
        shutdown();
        // 自旋（短 sleep）至在途任务清空，避免悬挂（同 task_container 析构）。
        // 注意：不要从运行在同一底层执行器上的协程内析构本对象，否则可能死锁。
        while(!empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }

    /**
     * @brief 便捷提交：等价于 spawn_detached。返回是否成功入队。
     */
    auto dispatch(silicon::scheduler::task<void>&& work) -> bool {
        return spawn_detached(std::move(work));
    }

    /**
     * @brief concepts::executor 要求：提交任务到工作队列，由 worker 取出运行。
     * @return 是否成功入队（已 shutdown 或底层执行器拒绝时返回 false）。
     */
    auto spawn_detached(silicon::scheduler::task<void>&& work) -> bool {
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
     * @brief concepts::executor 要求：提交任务，返回可 co_await 的 join 任务。
     * @return 一个在用户任务完成后置位的协程；入队失败时立即可完成（不悬挂）。
     */
    auto spawn_joinable(silicon::scheduler::task<void>&& work)
            -> silicon::scheduler::task<void> {
        auto e = std::make_shared<silicon::coroutine::event>();
        auto wrapper = [this, e, w = std::move(work)]() mutable -> silicon::scheduler::task<void> {
            try {
                co_await std::move(w);
            } catch(...) {
                capture_error();
            }
            e->set();
        };
        if(!spawn_detached(wrapper())) {
            e->set(); // 入队失败则立即放行 join，避免悬挂。
        }
        return [e]() -> silicon::scheduler::task<void> {
            co_await *e;
        }();
    }

    /**
     * @return 当前在途（排队 + 运行）任务数。
     */
    [[nodiscard]] auto size() const -> std::size_t {
        return m_p->m_inflight.load(std::memory_order::acquire);
    }

    /**
     * @return 是否无在途任务。
     */
    [[nodiscard]] auto empty() const -> bool { return size() == 0; }

    /**
     * @brief co_await 直到所有在途任务完成。worker 继续常驻，可继续 dispatch。
     */
    auto join() -> silicon::scheduler::task<void> {
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
    auto shutdown() -> void {
        if(m_p->m_stopped.exchange(true, std::memory_order::acq_rel)) {
            return;
        }
        (void)m_p->m_executor->spawn_detached(async_close());
    }

    /// @brief concepts::executor 要求：调度委托给底层执行器。
    auto schedule() { return m_p->m_executor->schedule(); }
    /// @brief concepts::executor 要求：让出委托给底层执行器。
    auto yield() { return m_p->m_executor->yield(); }
    /// @brief concepts::executor 要求：恢复句柄委托给底层执行器。
    auto resume(std::coroutine_handle<> handle) -> bool { return m_p->m_executor->resume(handle); }

    /**
     * @brief 最后捕获的未处理异常（worker 内任务抛错时记录，不重抛以免击垮 worker）。
     */
    [[nodiscard]] auto last_error() const -> std::exception_ptr { return m_p->m_last_error; }

  private:
    auto capture_error() -> void {
        if(!m_p->m_last_error) {
            m_p->m_last_error = std::current_exception();
        }
    }

    // 常驻 worker：循环取任务 -> 复用本协程帧运行 -> 循环；通道关闭且排空后退出。
    auto worker() -> silicon::scheduler::task<void> {
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
    }

    // 生产者协程：把任务送入通道（通道满时挂起，由消费者腾槽后唤醒）。
    auto sender(silicon::scheduler::task<void> work) -> silicon::scheduler::task<void> {
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
    auto async_close() -> silicon::scheduler::task<void> {
        while(m_p->m_pending_sends.load(std::memory_order::acquire) > 0) {
            co_await m_p->m_executor->yield();
        }
        co_await m_p->m_channel.close();
    }

    struct Impl {
      public:
        explicit Impl(std::size_t capacity)
            : m_channel{capacity} {}

        std::shared_ptr<Executor> m_executor{nullptr};
        channel<silicon::scheduler::task<void>> m_channel;
        std::atomic<std::size_t> m_inflight{0};
        std::atomic<std::size_t> m_pending_sends{0};
        std::atomic<bool> m_stopped{false};
        std::exception_ptr m_last_error{nullptr};
    };

    std::unique_ptr<Impl> m_p;
};

} // namespace silicon::coroutine
