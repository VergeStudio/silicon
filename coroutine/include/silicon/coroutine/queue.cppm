module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <optional>
#include <memory>
#include <utility>
#include <coroutine>
#include <atomic>
#include <queue>

export module silicon.coroutine:queue;

import silicon.scheduler;
import :mutex;
import silicon.scheduler.task;
export namespace silicon::coroutine {

enum class queue_produce_result {
    /**
     * @brief The item was successfully produced.
     */
    kProduced,
    /**
     * @brief The queue is shutting down or stopped, no more items are allowed to be produced.
     */
    kStopped
};

enum class queue_consume_result {
    /**
     * @brief The queue has shut down/stopped and the user should stop calling pop().
     */
    kStopped,

    /**
     * @brief try_pop() failed to acquire the lock.
     */
    kTryLockFailure,

    /**
     * @brief try_pop() acquired the lock but there are no items in the queue.
     */
    kEmpty,
};

/**
 * @brief An unbounded queue. If the queue is empty and there are waiters to consume then
 *        there are no allocations and the coroutine context will simply be passed to the
 *        waiter. If there are no waiters the item being produced will be placed into the
 *        queue.
 *
 * @tparam element_type The type of items being produced and consumed.
 *
 * @note 实现（成员函数体）位于 coroutine/src/queue.cpp（主模块实现单元），
 *       本接口分区仅保留声明与嵌套类型布局，以降低编辑实现时的重编波及面。
 */
template<typename element_type>
class queue {
  private:
    enum class running_state_t {
        kRunning,
        kDraining,
        kStopped,
    };

  public:
    struct awaiter {
        explicit awaiter(queue<element_type> &q) noexcept;

        auto await_ready() noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
        [[nodiscard]] auto await_resume() noexcept -> expected<element_type, queue_consume_result>;

        std::optional<element_type> m_element{std::nullopt};
        queue &m_queue;
        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        awaiter *m_next{nullptr};
    };

    queue();
    ~queue();

    queue(const queue &) = delete;
    queue(queue &&other) = delete;

    auto operator=(const queue &) -> queue & = delete;
    auto operator=(queue &&other) -> queue & = delete;

    /**
     * @brief Determines if the queue is empty.
     *
     * @return true If the queue is empty.
     * @return false If the queue is not empty.
     */
    auto empty() const -> bool;

    /**
     * @brief Gets the number of elements in the queue.
     *
     * @return std::size_t The number of elements in the queue.
     */
    auto size() const -> std::size_t;

    /**
     * @brief Pushes the element into the queue. If the queue is empty and there are waiters
     *        then the element will be processed immediately by transfering the coroutine task
     *        context to the waiter.
     *
     * @param element The element being produced.
     * @return silicon::scheduler::task<queue_produce_result>
     */
    auto push(const element_type &element) -> silicon::scheduler::task<queue_produce_result>;

    /**
     * @brief Pushes the element into the queue. If the queue is empty and there are waiters
     *        then the element will be processed immediately by transfering the coroutine task
     *        context to the waiter.
     *
     * @param element The element being produced.
     * @return silicon::scheduler::task<queue_produce_result>
     */
    auto push(element_type &&element) -> silicon::scheduler::task<queue_produce_result>;

    /**
     * @brief Emplaces an element into the queue. Has the same behavior as push if the queue
     *        is empty and has waiters.
     *
     * @param args The element's constructor argument types and values.
     * @return silicon::scheduler::task<queue_produce_result>
     */
    template<typename... args_type>
    auto emplace(args_type &&...args) -> silicon::scheduler::task<queue_produce_result>;

    /**
     * @brief Pops the head element of the queue if available, or waits for one to be available.
     *
     * @return awaiter A waiter task that upon co_await complete returns an element or the queue
     *                 status that it is shut down.
     */
    [[nodiscard]] auto pop() -> silicon::scheduler::task<expected<element_type, queue_consume_result>>;

    /**
     * @brief Tries to pop the head element of the queue if available. This can fail if it cannot
     *        acquire the lock via `silicon::coroutine::mutex::try_lock()` or if there are no elements available.
     *        Does not block.
     *
     * @return expected<element_type, queue_consume_result> The head element if the lock was acquired
     *         and an element is available.
     *         queue_consume_result::stopped if the queue has been shutdown.
     *         queue_consume_result::empty if lock was acquired but the queue is empty.
     *         queue_consume_result::try_lock_failure if the queue is in use and the lock could not be acquired.
     */
    [[nodiscard]] auto try_pop() -> expected<element_type, queue_consume_result>;

    /**
     * @brief Shuts down the queue immediately discarding any elements that haven't been processed.
     *
     * @return silicon::scheduler::task<void>
     */
    auto shutdown() -> silicon::scheduler::task<void>;

    /**
     * @brief Shuts down the queue but waits for it to be drained so all elements are processed.
     *        Will yield on the given executor between checking if the queue is empty so the tasks
     *        can be processed.
     *
     * @tparam executor_t The executor type.
     * @param e The executor to yield this task to wait for elements to be processed.
     * @return silicon::scheduler::task<void>
     */
    template<silicon::coroutine::concepts::executor executor_type>
    auto shutdown_drain(std::unique_ptr<executor_type> &e) -> silicon::scheduler::task<void>;

    /**
     * Returns true if shutdown() or shutdown_drain() have been called on this silicon::coroutine::queue.
     * @return True if the silicon::coroutine::queue has been shutdown.
     */
    [[nodiscard]] auto is_shutdown() const -> bool;

  private:
    friend awaiter;

    struct Impl {
      public:
        /// @brief The list of pop() awaiters.
        awaiter *m_waiters{nullptr};
        /// @brief Mutex for properly maintaining the queue.
        silicon::coroutine::mutex m_mutex{};
        /// @brief The underlying queue data structure.
        std::queue<element_type> m_elements{};
        /// @brief The current running state of the queue.
        std::atomic<running_state_t> m_running_state{running_state_t::kRunning};
    };

    std::unique_ptr<Impl> m_p;
};

} // namespace silicon::coroutine
