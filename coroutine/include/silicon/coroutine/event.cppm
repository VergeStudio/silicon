module;


#include <atomic>
#include <coroutine>
#include <memory>
#include <vector>


export module silicon.coroutine:event;

import silicon.scheduler;

export namespace silicon::coroutine {
enum class resume_order_policy {
    /// Last in first out, this is the default policy and will execute the fastest
    /// if you do not need the first waiter to execute first upon the event being set.
    kLifo,
    /// First in first out, this policy has an extra overhead to reverse the order of
    /// the waiters but will guarantee the ordering is fifo.
    kFifo
};

/**
 * event is a manually triggered thread safe signal that can be co_await()'ed by multiple awaiters.
 * Each awaiter should co_await the event and upon the event being set each awaiter will have their
 * coroutine resumed.
 *
 * The event can be manually reset to the un-set state to be re-used.
 * \code
t1: silicon::coroutine::event e;
...
t2: func(silicon::coroutine::event& e) { ... co_await e; ... }
...
t1: do_work();
t1: e.set();
...
t2: resume()
 * \endcode
 */
class event {
  public:
    struct awaiter {
        /**
         * @param e The event to wait for it to be set.
         */
        awaiter(const event &e) noexcept: m_event(e) {}

        /**
         * @return True if the event is already set, otherwise false to suspend this coroutine.
         */
        bool await_ready() const noexcept { return m_event.is_set(); }

        /**
         * Adds this coroutine to the list of awaiters in a thread safe fashion.  If the event
         * is set while attempting to add this coroutine to the awaiters then this will return false
         * to resume execution immediately.
         * @return False if the event is already set, otherwise true to suspend this coroutine.
         */
        bool await_suspend(std::coroutine_handle<>) noexcept ;

        /**
         * Nothing to do on resume.
         */
        auto await_resume() noexcept {}

        /// @brief The next awaiter in line for this event, nullptr if this is the end.
        awaiter *m_next{nullptr};
        /// @brief The awaiting continuation coroutine handle.
        std::coroutine_handle<> m_awaiting_coroutine;
        /// Refernce to the event that this awaiter is waiting on.
        const event &m_event;
    };

    /**
     * Creates an event with the given initial state of being set or not set.
     * @param initially_set By default all events start as not set, but if needed this parameter can
     *                      set the event to already be triggered.
     */
    explicit event(bool = false) noexcept;
    ~event();

    event(const event &) = delete;
    event(event &&) = delete;
    event & operator=(const event &) = delete;
    event & operator=(event &&) = delete;

    /**
     * @return True if this event is currently in the set state.
     */
    bool is_set() const noexcept ;

    /**
     * Sets this event and resumes all awaiters.  Note that all waiters will be resumed onto this
     * thread of execution.
     * @param policy The order in which the waiters should be resumed, defaults to LIFO since it
     *               is more efficient, FIFO requires reversing the order of the waiters first.
     */
    void set(resume_order_policy = resume_order_policy::kLifo) noexcept ;

    /**
     * Sets this event and resumes all awaiters onto the given executor.  This will distribute
     * the waiters across the executor's threads.
     */
    template<concepts::executor executor_type>
    void set(std::unique_ptr<executor_type> &e, resume_order_policy policy = resume_order_policy::kLifo) noexcept {
        void *old_value = exchange_set_state();
        if(old_value != this) {
            // If FIFO has been requested then reverse the order upon resuming.
            if(policy == resume_order_policy::kFifo) {
                old_value = reverse(static_cast<awaiter *>(old_value));
            }
            // else lifo nothing to do

            auto *waiters = static_cast<awaiter *>(old_value);
            while(waiters != nullptr) {
                auto *next = waiters->m_next;
                e->resume(waiters->m_awaiting_coroutine);
                waiters = next;
            }
        }
    }

    /**
     * @return An awaiter struct to suspend and resume this coroutine for when the event is set.
     */
    auto operator co_await() const noexcept -> awaiter { return awaiter(*this); }

    /**
     * Resets the event from set to not set so it can be re-used.  If the event is not currently
     * set then this function has no effect.
     */
    void reset() noexcept ;

  private:
    /// For access to m_p.
    friend struct awaiter;

    /// PIMPL：impl 仅前置声明，定义置于 src/event.cpp。
    struct impl;
    /// Hidden implementation state.
    std::unique_ptr<impl> m_p;

    /**
     * Reverses the set of waiters from LIFO->FIFO and returns the new head.
     */
    auto reverse(awaiter *head) -> awaiter *;

    /**
     * 非模板钩子：把状态原子交换为 this 并返回旧值。
     * 供接口单元中的 `set(executor)` 模板重载使用，避免 impl 泄漏到接口单元。
     */
    void * exchange_set_state() noexcept ;
};

} // namespace silicon::coroutine
