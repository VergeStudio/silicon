module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <memory>


#include <coroutine>


#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <optional>
#include <utility>

#include <tuple>
#include <silicon/proxy/proxy_macros.h>


#ifdef LIBCORO_FEATURE_NETWORKING
#    include <stop_token>
#endif

// latest gcc versions > 13 are incorrectly reporting the std::optional<std::function<bool>()> = std::nullopt as the internal
// std::function being uninitialized, it is but that is expected since its wrapped in an nullopt optional.
#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

export module silicon.coroutine:facade;

import silicon.scheduler;
import silicon.scheduler.task;
import :event;
import :mutex;
import :when_any;
import silicon.proxy;

export namespace silicon::coroutine {

class condition_variable {
  public:
    using predicate_type = std::function<bool()>;

  private:
    enum class notify_status_t {
        /// @brief The waiter is ready to be resumed, either the predicate passed or its been requested to stop.
        kReady,
        /// @brief The waiter is not ready to be resumed
        kNotReady,
        /// @brief The waiter is a hook and has timed out and is dead.
        kAwaiterDead,
    };

    // on_notify 接口：类型擦除门面
    // awaiter_base 仍作为侵入式链表节点（m_next）被以 awaiter_base* 持有，
    // on_notify 经内部类型擦除 strategy_ 分派到具体 awaiter 的 do_on_notify。
    PRO_DEF_MEM_DISPATCH(MemNotify, on_notify);

    struct notify_facade
        : silicon::proxy::facade_builder
          ::add_convention<MemNotify, silicon::scheduler::task<notify_status_t>()>
          ::build {};

    using notify_proxy = silicon::proxy::proxy<notify_facade>;
    using notify_view = silicon::proxy::proxy_view<notify_facade>;

    template<class T, class... Args>
    [[nodiscard]] notify_proxy make_notify(Args &&...args) {
        return silicon::proxy::make_proxy<notify_facade, T>(std::forward<Args>(args)...);
    }

    template<class T>
    [[nodiscard]] notify_view make_notify_view(T &target) noexcept {
        return silicon::proxy::make_proxy_view<notify_facade>(target);
    }

    // 适配器：把具体 awaiter 的 do_on_notify 桥接到 notify_facade。
    template<class Awaiter>
    struct notify_strategy {
        Awaiter* self;
        silicon::scheduler::task<notify_status_t> on_notify() { return self->do_on_notify(); }
    };

    struct awaiter_base {
        awaiter_base(silicon::coroutine::condition_variable &cv, silicon::coroutine::scoped_lock &l);
        ~awaiter_base() = default;

        awaiter_base(const awaiter_base &) = delete;
        awaiter_base(awaiter_base &&) = delete;
        auto operator=(const awaiter_base &) -> awaiter_base & = delete;
        auto operator=(awaiter_base &&) -> awaiter_base & = delete;

        /// @brief The next waiting awaiter.
        awaiter_base *m_next{nullptr};
        /// @brief The coroutine to resume the waiter.
        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        /// @brief The condition variable this waiter is waiting on.
        silicon::coroutine::condition_variable &m_condition_variable;
        /// @brief The lock that the wait() was called with.
        silicon::coroutine::scoped_lock &m_lock;

        /// @brief 类型擦除 on_notify：经 strategy_ 分派到具体 awaiter 的 do_on_notify。
        notify_proxy strategy_{};
        auto on_notify() -> silicon::scheduler::task<notify_status_t> {
            return strategy_.on_notify();
        }
    };

    struct awaiter: public awaiter_base {
        awaiter(silicon::coroutine::condition_variable &cv, silicon::coroutine::scoped_lock &l) noexcept;
        ~awaiter() = default;

        awaiter(const awaiter &) = delete;
        awaiter(awaiter &&) = delete;
        auto operator=(const awaiter &) -> awaiter & = delete;
        auto operator=(awaiter &&) -> awaiter & = delete;

        auto await_ready() const noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
        auto await_resume() noexcept {}

        auto do_on_notify() -> silicon::scheduler::task<notify_status_t>;
    };

    struct awaiter_with_predicate: public awaiter_base {
        awaiter_with_predicate(silicon::coroutine::condition_variable &cv, silicon::coroutine::scoped_lock &l, predicate_type p) noexcept;
        ~awaiter_with_predicate() = default;

        awaiter_with_predicate(const awaiter_with_predicate &) = delete;
        awaiter_with_predicate(awaiter_with_predicate &&) = delete;
        auto operator=(const awaiter_with_predicate &) -> awaiter_with_predicate & = delete;
        auto operator=(awaiter_with_predicate &&) -> awaiter_with_predicate & = delete;

        auto await_ready() const noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
        auto await_resume() noexcept {}

        auto do_on_notify() -> silicon::scheduler::task<notify_status_t>;

        /// @brief The wait predicate to execute on notify.
        predicate_type m_predicate;
    };

#ifndef EMSCRIPTEN

    struct awaiter_with_predicate_stop_token: public awaiter_base {
        awaiter_with_predicate_stop_token(
                silicon::coroutine::condition_variable &cv, silicon::coroutine::scoped_lock &l, predicate_type p, std::stop_token stop_token
        ) noexcept;
        ~awaiter_with_predicate_stop_token() = default;

        awaiter_with_predicate_stop_token(const awaiter_with_predicate_stop_token &) = delete;
        awaiter_with_predicate_stop_token(awaiter_with_predicate_stop_token &&) = delete;
        auto operator=(const awaiter_with_predicate_stop_token &) -> awaiter_with_predicate_stop_token & = delete;
        auto operator=(awaiter_with_predicate_stop_token &&) -> awaiter_with_predicate_stop_token & = delete;

        auto await_ready() noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
        auto await_resume() noexcept -> bool { return m_predicate_result; }

        auto do_on_notify() -> silicon::scheduler::task<notify_status_t>;

        /// @brief The wait predicate to execute on notify.
        predicate_type m_predicate;
        /// @brief The stop token that will guarantee the next notify will wake the awaiter regardless of the predicate.
        std::stop_token m_stop_token;
        /// @brief The last predicate's call result.
        bool m_predicate_result{false};
    };

#endif

#ifdef LIBCORO_FEATURE_NETWORKING

    /// @brief This structure encapsulates the data from the controller task.
    struct controller_data {
        controller_data(
                std::optional<std::cv_status> &status,
                bool &predicate_result,
                std::optional<predicate_type> predicate,
                std::optional<const std::stop_token> stop_token
        ) noexcept;
        ~controller_data() = default;

        controller_data(const controller_data &) = delete;
        controller_data(controller_data &&) = delete;
        auto operator=(const controller_data &) -> controller_data & = delete;
        auto operator=(controller_data &&) -> controller_data & = delete;

        /// @brief Mutex for notify or timeout mutual exclusion.
        silicon::coroutine::mutex m_event_mutex{};
        /// @brief event to notify the no timeout task.
        silicon::coroutine::event m_notify_callback{};
        /// @brief Flag to notify if the awaiter has completed via no_timeout or timeout.
        std::atomic<bool> m_awaiter_completed{false};
        /// @brief The status of the wait() call, this is _ONLY_ valid if the m_awaiter_completed flag was obtained,
        /// otherwise its a dangling reference.
        std::optional<std::cv_status> &m_status;
        /// @brief The last result of the predicate call.
        bool &m_predicate_result;
        /// @brief The predicate, this can be no predicate, default predicate or stop token predicate.
        std::optional<predicate_type> m_predicate{std::nullopt};
        /// @brief The stop token.
        std::optional<const std::stop_token> m_stop_token{std::nullopt};
    };

    /**
     * @brief This awaiter is a facade to hook in between the wait_[for|unitl]() caller and the actual awaiter object.
     * This allows the hook to either call the no_timeout or timeout results correctly based on which one completes
     * first.
     *
     * This class mimics the awaiter_base class so it can silently sit in the coro:condition_variable.m_awaiters to look
     * like its the actual awaiter, but just proxies to the real awaiter when appropriate.
     *
     * If the wait_[for|until] does timeout, then this will do nothing except delete itself from the list on a
     * notify_[one|all] call.
     */
    struct awaiter_with_wait_hook: public awaiter_base {
        awaiter_with_wait_hook(silicon::coroutine::condition_variable &cv, silicon::coroutine::scoped_lock &l, controller_data &data) noexcept;
        ~awaiter_with_wait_hook() = default;

        auto do_on_notify() -> silicon::scheduler::task<notify_status_t>;

        controller_data &m_data;
    };

    template<concepts::io_executor io_executor_type, typename return_type>
    struct awaiter_with_wait: public awaiter_base {
        awaiter_with_wait(
                std::unique_ptr<io_executor_type> &executor,
                silicon::coroutine::condition_variable &cv,
                silicon::coroutine::scoped_lock &l,
                const std::chrono::nanoseconds wait_for,
                std::optional<predicate_type> predicate = std::nullopt,
                std::optional<std::stop_token> stop_token = std::nullopt
        ) noexcept
            : awaiter_base(cv, l),
              m_executor(executor),
              m_wait_for(wait_for),
              m_predicate(std::move(predicate)),
              m_stop_token(std::move(stop_token)) {
            strategy_ = make_notify<notify_strategy<awaiter_with_wait>>(this);
        }
        ~awaiter_with_wait() = default;

        awaiter_with_wait(const awaiter_with_wait &) = delete;
        awaiter_with_wait(awaiter_with_wait &&) = delete;
        auto operator=(const awaiter_with_wait &) -> awaiter_with_wait & = delete;
        auto operator=(awaiter_with_wait &&) -> awaiter_with_wait & = delete;

        /**
         * @brief Task to handle the no_timeout case, however it is resumed even after a timeout since it needs to exit
         * for the controller task.
         *
         * @param data The controller task's data.
         * @return silicon::scheduler::task<void>
         */
        auto make_on_notify_callback_task(controller_data &data) -> silicon::scheduler::task<void> {
            co_await data.m_notify_callback;

            // If this is the condition and not a timeout resume from this task.
            if(m_status.value() == std::cv_status::no_timeout) {
                // The condition coroutine is resumed from here instead of the awaiter hook task to
                // guarantee the controller is still alive until the caller's coroutine is completed.
                m_awaiting_coroutine.resume();
            }

            // This was a timeout, do nothing but exit to let the controller task know it can safely exit now.
            co_return;
        }

        /**
         * @brief Task to handle the timeout case, this will always wait the duration of the timeout before exiting.
         *
         * @param data The controller task data.
         * @return silicon::scheduler::task<void>
         */
        auto make_timeout_task(controller_data &data) -> silicon::scheduler::task<void> {
            co_await m_executor->schedule_after(m_wait_for);
            auto lock = co_await data.m_event_mutex.scoped_lock();
            bool expected{false};
            if(data.m_awaiter_completed.compare_exchange_strong(
                       expected, true, std::memory_order::release, std::memory_order::relaxed
               )) {
                m_status = {std::cv_status::timeout};
                lock.unlock();

                // This means the timeout has occurred first. Before resuming the wait_[for|until]() caller the lock
                // must be re-acquired.
                co_await m_lock.owned_mutex()->lock();
                m_predicate_result = data.m_predicate.has_value() ? data.m_predicate.value()() : true;
                m_awaiting_coroutine.resume();
                co_return;
            }

            // The no_timeout has occurred first, exit so the controller task can complete.
            co_return;
        }

        /**
         * @brief Task to manage the no_timeout and timeout tasks, this holds the state for both tasks since they need
         * to reference the actual wait_[for|until]() awaiter, but need to do so safely while it is still alive. To
         * access the true awaiter the awaiter_completed atomic bool must be acquired, if it is not acquired the calling
         * awaiter is invalid since it has already been resumed with the first event of timeout or no_timeout.
         *
         * @return silicon::scheduler::task_self_deleting This task is self deleting since it has an indeterminate lifetime.
         */
        auto make_controller_task() -> silicon::scheduler::task_self_deleting {
            controller_data data{m_status, m_predicate_result, std::move(m_predicate), std::move(m_stop_token)};
            // We enqueue the hook_task since we can make it live until the notify occurs and will properly resume the
            // actual coroutine only once.
            awaiter_with_wait_hook hook_task{m_condition_variable, m_lock, data};
            m_condition_variable.push_waiter(static_cast<awaiter_base *>(&hook_task));
            m_lock.owned_mutex()->unlock(); // Unlock the actual lock now that we are setup, not the fake hook task.

            co_await silicon::coroutine::when_all(make_on_notify_callback_task(data), make_timeout_task(data));
            co_return;
        }

        auto await_ready() noexcept -> bool {
            // If there is no predicate then we are not ready.
            if(!m_predicate.has_value()) {
                return false;
            }

            m_predicate_result = m_predicate.value()();
            if(m_predicate_result && m_stop_token.has_value() && m_stop_token.value().stop_requested()) {
                m_status = std::cv_status::no_timeout;
            }
            return m_predicate_result;
        }

        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
            m_awaiting_coroutine = awaiting_coroutine; // This is the real coroutine to resume.

            // Make the background controller which proxies between the notify task and the timeout task.
            auto controller = make_controller_task();
            controller.resume();
            return true;
        }

        auto await_resume() noexcept -> return_type {
            if constexpr(std::is_same_v<return_type, bool>) {
                return m_predicate_result;
            } else {
                return m_status.value();
            }
        }

        auto do_on_notify() -> silicon::scheduler::task<notify_status_t> { std::unreachable(); }

        /// @brief The io_executor used to wait for the timeout.
        std::unique_ptr<io_executor_type> &m_executor;
        /// @brief The amount of time to wait for before timing out.
        const std::chrono::nanoseconds m_wait_for;
        /// @brief If the condition timed out or not.
        std::optional<std::cv_status> m_status{std::nullopt};
        /// @brief The last m_predicate() call result.
        bool m_predicate_result{false};
        /// @brief The predicate, this can be no predicate, default predicate or stop token predicate. This value is
        /// only valid until the awaiter data object takes ownership.
        std::optional<predicate_type> m_predicate{std::nullopt};
        /// @brief The stop token. This value is only valid until the awater data object takes onwership.
        std::optional<const std::stop_token> m_stop_token{std::nullopt};
    };

#endif

  public:
    condition_variable();
    ~condition_variable();

    condition_variable(const condition_variable &) = delete;
    condition_variable(condition_variable &&) = delete;
    auto operator=(const condition_variable &) -> condition_variable & = delete;
    auto operator=(condition_variable &&) -> condition_variable & = delete;

    /**
     * @brief Notifies a single waiter.
     */
    auto notify_one() -> silicon::scheduler::task<void>;

    /**
     * @brief Notifies a single waiter and resumes the waiter on the given executor.
     *
     * @tparam executor_type The type of executor that the waiter will be resumed on.
     * @param executor The executor that the waiter will be resumed on.
     */
    template<silicon::coroutine::concepts::executor executor_type>
    auto notify_one(std::unique_ptr<executor_type> &executor) -> void {
        executor->spawn_detached(notify_one());
    }

    /**
     * @brief Notifies all waiters.
     */
    auto notify_all() -> silicon::scheduler::task<void>;

    /**
     * @brief Notifies all waiters and resumes them on the given executor. Note that each waiter must be notified
     * synchronously so this is useful if the task is long lived and can be immediately parallelized after the condition
     * is ready. This does not need to be co_await'ed like `notify_all()` since this will execute the notify on the
     * given executor.
     *
     * @tparam executor_type The type of executor that the waiters will be resumed on.
     * @param executor The executor that each waiter will be resumed on.
     * @return void
     */
    template<silicon::coroutine::concepts::executor executor_type>
    auto notify_all(std::unique_ptr<executor_type> &executor) -> void {
        auto *waiter = pop_all_waiters();

        while(waiter != nullptr) {
            // Need to grab next before notifying since the notifier will self destruct after completing.
            awaiter_base *next = waiter->m_next;
            // This will kick off each task in parallel on the scheduler, they will fight over the lock
            // but will give the best parallelism scheduling them immediately.
            executor->spawn_detached(make_notify_all_executor_individual_task(waiter));
            waiter = next;
        }

        return;
    }

    /**
     * @brief Waits until notified.
     *
     * @param lock A lock that must be locked by the caller.
     * @return awaiter
     */
    [[nodiscard]] auto wait(silicon::coroutine::scoped_lock &lock) -> awaiter;

    /**
     * @brief Waits until notified but only wakes up if the predicate passes.
     *
     * @param lock A lock that must be locked by the caller.
     * @param predicate The predicate to check whether the waiting can be completed.
     * @return awaiter_with_predicate
     */
    [[nodiscard]] auto wait(silicon::coroutine::scoped_lock &lock, predicate_type predicate) -> awaiter_with_predicate;

#ifndef EMSCRIPTEN
    /**
     * @brief Waits until notified and wakes up if a stop is requseted or the predicate passes.
     *
     * @param lock A lock which must be locked by the caller.
     * @param stop_token A stop token to register interruption for.
     * @param predicate The predicate to check whether the waiting can be completed.
     * @return awaiter_with_predicate_stop_token The final predicate call result.
     */
    [[nodiscard]] auto wait(silicon::coroutine::scoped_lock &lock, std::stop_token stop_token, predicate_type predicate)
            -> awaiter_with_predicate_stop_token;
#endif

#ifdef LIBCORO_FEATURE_NETWORKING

    template<concepts::io_executor io_executor_type, class rep_type, class period_type>
    [[nodiscard]] auto wait_for(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            const std::chrono::duration<rep_type, period_type> wait_for
    )
            -> awaiter_with_wait<io_executor_type, std::cv_status> {
        return awaiter_with_wait<io_executor_type, std::cv_status>{
                executor, *this, lock, std::chrono::duration_cast<std::chrono::nanoseconds>(wait_for)
        };
    }

    template<concepts::io_executor io_executor_type, class rep_type, class period_type>
    [[nodiscard]] auto wait_for(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            const std::chrono::duration<rep_type, period_type> wait_for,
            predicate_type predicate
    ) -> awaiter_with_wait<io_executor_type, bool> {
        return awaiter_with_wait<io_executor_type, bool>{
                executor,
                *this,
                lock,
                std::chrono::duration_cast<std::chrono::nanoseconds>(wait_for),
                std::move(predicate)
        };
    }

    template<concepts::io_executor io_executor_type, class rep_type, class period_type>
    [[nodiscard]] auto wait_for(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            std::stop_token stop_token,
            const std::chrono::duration<rep_type, period_type> wait_for,
            predicate_type predicate
    ) -> awaiter_with_wait<io_executor_type, bool> {
        return awaiter_with_wait<io_executor_type, bool>{
                executor,
                *this,
                lock,
                std::chrono::duration_cast<std::chrono::nanoseconds>(wait_for),
                std::move(predicate),
                std::move(stop_token)
        };
    }

    template<concepts::io_executor io_executor_type, class clock_type, class duration_type>
    auto wait_until(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            const std::chrono::time_point<clock_type, duration_type> wait_until_time
    )
            -> awaiter_with_wait<io_executor_type, std::cv_status> {
        auto now = std::chrono::time_point<clock_type, duration_type>::clock::now();
        auto wait_for = (now < wait_until_time) ? (wait_until_time - now) : std::chrono::nanoseconds{1};
        return awaiter_with_wait<io_executor_type, std::cv_status>{
                executor, *this, lock, std::chrono::duration_cast<std::chrono::nanoseconds>(wait_for)
        };
    }

    template<concepts::io_executor io_executor_type, class clock_type, class duration_type>
    auto wait_until(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            const std::chrono::time_point<clock_type, duration_type> wait_until_time,
            predicate_type predicate
    ) -> awaiter_with_wait<io_executor_type, bool> {
        auto now = std::chrono::time_point<clock_type, duration_type>::clock::now();
        auto wait_for = (now < wait_until_time) ? (wait_until_time - now) : std::chrono::nanoseconds{1};
        return awaiter_with_wait<io_executor_type, bool>{
                executor,
                *this,
                lock,
                std::chrono::duration_cast<std::chrono::nanoseconds>(wait_for),
                std::move(predicate)
        };
    }

    template<concepts::io_executor io_executor_type, class clock_type, class duration_type>
    auto wait_until(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            std::stop_token stop_token,
            const std::chrono::time_point<clock_type, duration_type> wait_until_time,
            predicate_type predicate
    ) -> awaiter_with_wait<io_executor_type, bool> {
        auto now = std::chrono::time_point<clock_type, duration_type>::clock::now();
        auto wait_for = (now < wait_until_time) ? (wait_until_time - now) : std::chrono::nanoseconds{1};
        return awaiter_with_wait<io_executor_type, bool>{
                executor,
                *this,
                lock,
                std::chrono::duration_cast<std::chrono::nanoseconds>(wait_for),
                std::move(predicate),
                std::move(stop_token)
        };
    }
#endif

  private:
    /// Implementation state, fully hidden in the implementation unit.
    struct impl;
    std::unique_ptr<impl> m_p;

    /**
     * @brief Non-template hooks over the hidden waiter list.
     *
     * The templated notify_*(executor) overloads and the templated wait_[for|until]() awaiters live in
     * this interface unit but still need to mutate the waiter list held by impl.  Routing those accesses
     * through non-template members keeps impl defined only in condition_variable.cpp.
     */
    /// @brief Pops the entire waiter list, the caller owns the returned intrusive list.
    auto pop_all_waiters() noexcept -> awaiter_base *;
    /// @brief Pushes a waiter back onto the waiter list.
    auto push_waiter(awaiter_base *waiter) noexcept -> void;

    auto make_notify_all_executor_individual_task(awaiter_base *waiter) -> silicon::scheduler::task<void>;
};


#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif

} // namespace silicon::coroutine
