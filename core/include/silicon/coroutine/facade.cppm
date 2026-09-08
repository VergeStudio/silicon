module;

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

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include <silicon/common.h>
export module silicon.coroutine:facade;

import silicon.scheduler;
import silicon.scheduler.task;
import :event;
import :mutex;
import :when_any;
import silicon.proxy;

export namespace silicon::coroutine {

class SILICON_CORE_API condition_variable {
  public:

  private:
    enum class notify_status_t {

        kReady,

        kNotReady,

        kAwaiterDead,
    };

    PRO_DEF_MEM_DISPATCH(MemNotify, on_notify);

    struct notify_facade
        : silicon::proxy::facade_builder
          ::add_convention<MemNotify, silicon::scheduler::task<notify_status_t>()>
          ::build {};


    template<class T, class... Args>
    [[nodiscard]] static silicon::proxy::proxy<notify_facade> make_notify(Args &&...args) {
        return silicon::proxy::make_proxy<notify_facade, T>(std::forward<Args>(args)...);
    }

    template<class T>
    [[nodiscard]] static silicon::proxy::proxy_view<notify_facade> make_notify_view(T &target) noexcept {
        return silicon::proxy::make_proxy_view<notify_facade>(target);
    }

    template<class Awaiter>
    struct notify_strategy {
        Awaiter* self;
        silicon::scheduler::task<notify_status_t> on_notify() { return self->do_on_notify(); }
    };

    struct awaiter_base {
        awaiter_base(silicon::coroutine::condition_variable &, silicon::coroutine::scoped_lock &);
        ~awaiter_base() = default;

        awaiter_base(const awaiter_base &) = delete;
        awaiter_base(awaiter_base &&) = delete;
        awaiter_base & operator=(const awaiter_base &) = delete;
        awaiter_base & operator=(awaiter_base &&) = delete;

        awaiter_base *m_next{nullptr};

        std::coroutine_handle<> m_awaiting_coroutine{nullptr};

        silicon::coroutine::condition_variable &m_condition_variable;

        silicon::coroutine::scoped_lock &m_lock;

        silicon::proxy::proxy<notify_facade> strategy_{};
        silicon::scheduler::task<notify_status_t> on_notify() {
            return strategy_->on_notify();
        }
    };

    struct awaiter: public awaiter_base {
        awaiter(silicon::coroutine::condition_variable &, silicon::coroutine::scoped_lock &) noexcept;
        ~awaiter() = default;

        awaiter(const awaiter &) = delete;
        awaiter(awaiter &&) = delete;
        auto operator=(const awaiter &) -> awaiter & = delete;
        auto operator=(awaiter &&) -> awaiter & = delete;

        bool await_ready() const noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;
        auto await_resume() noexcept {}

        silicon::scheduler::task<notify_status_t> do_on_notify() ;
    };

    struct awaiter_with_predicate: public awaiter_base {
        awaiter_with_predicate(silicon::coroutine::condition_variable &, silicon::coroutine::scoped_lock &, std::function<bool()>) noexcept;
        ~awaiter_with_predicate() = default;

        awaiter_with_predicate(const awaiter_with_predicate &) = delete;
        awaiter_with_predicate(awaiter_with_predicate &&) = delete;
        awaiter_with_predicate & operator=(const awaiter_with_predicate &) = delete;
        awaiter_with_predicate & operator=(awaiter_with_predicate &&) = delete;

        bool await_ready() const noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;
        auto await_resume() noexcept {}

        silicon::scheduler::task<notify_status_t> do_on_notify() ;

        std::function<bool()> m_predicate;
    };

#ifndef EMSCRIPTEN

    struct awaiter_with_predicate_stop_token: public awaiter_base {
        awaiter_with_predicate_stop_token(
                silicon::coroutine::condition_variable &, silicon::coroutine::scoped_lock &, std::function<bool()>, std::stop_token
        ) noexcept;
        ~awaiter_with_predicate_stop_token() = default;

        awaiter_with_predicate_stop_token(const awaiter_with_predicate_stop_token &) = delete;
        awaiter_with_predicate_stop_token(awaiter_with_predicate_stop_token &&) = delete;
        awaiter_with_predicate_stop_token & operator=(const awaiter_with_predicate_stop_token &) = delete;
        awaiter_with_predicate_stop_token & operator=(awaiter_with_predicate_stop_token &&) = delete;

        bool await_ready() noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;
        bool await_resume() noexcept { return m_predicate_result; }

        silicon::scheduler::task<notify_status_t> do_on_notify() ;

        std::function<bool()> m_predicate;

        std::stop_token m_stop_token;

        bool m_predicate_result{false};
    };

#endif

#ifdef LIBCORO_FEATURE_NETWORKING

    struct controller_data {
        controller_data(
                std::optional<std::cv_status> &,
                bool &,
                std::optional<std::function<bool()>>,
                std::optional<const std::stop_token>
        ) noexcept;
        ~controller_data() = default;

        controller_data(const controller_data &) = delete;
        controller_data(controller_data &&) = delete;
        controller_data & operator=(const controller_data &) = delete;
        controller_data & operator=(controller_data &&) = delete;

        silicon::coroutine::mutex m_event_mutex{};

        silicon::coroutine::event m_notify_callback{};

        std::atomic<bool> m_awaiter_completed{false};

        std::optional<std::cv_status> &m_status;

        bool &m_predicate_result;

        std::optional<std::function<bool()>> m_predicate{std::nullopt};

        std::optional<const std::stop_token> m_stop_token{std::nullopt};
    };

    struct awaiter_with_wait_hook: public awaiter_base {
        awaiter_with_wait_hook(silicon::coroutine::condition_variable &, silicon::coroutine::scoped_lock &, controller_data &) noexcept;
        ~awaiter_with_wait_hook() = default;

        silicon::scheduler::task<notify_status_t> do_on_notify() ;

        controller_data &m_data;
    };

    template<silicon::scheduler::concepts::io_executor io_executor_type, typename return_type>
    struct awaiter_with_wait: public awaiter_base {
        awaiter_with_wait(
                std::unique_ptr<io_executor_type> &executor,
                silicon::coroutine::condition_variable &cv,
                silicon::coroutine::scoped_lock &l,
                const std::chrono::nanoseconds wait_for,
                std::optional<std::function<bool()>> predicate = std::nullopt,
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
        awaiter_with_wait & operator=(const awaiter_with_wait &) = delete;
        awaiter_with_wait & operator=(awaiter_with_wait &&) = delete;

        silicon::scheduler::task<void> make_on_notify_callback_task(controller_data &data) {
            co_await data.m_notify_callback;

            if(m_status.value() == std::cv_status::no_timeout) {

                m_awaiting_coroutine.resume();
            }

            co_return;
        }

        silicon::scheduler::task<void> make_timeout_task(controller_data &data) {
            co_await m_executor->schedule_after(m_wait_for);
            auto lock = co_await data.m_event_mutex.scoped_lock();
            bool expected{false};
            if(data.m_awaiter_completed.compare_exchange_strong(
                       expected, true, std::memory_order::release, std::memory_order::relaxed
               )) {
                m_status = {std::cv_status::timeout};
                lock.unlock();

                co_await m_lock.owned_mutex()->lock();
                m_predicate_result = data.m_predicate.has_value() ? data.m_predicate.value()() : true;
                m_awaiting_coroutine.resume();
                co_return;
            }

            co_return;
        }

        silicon::scheduler::task_self_deleting make_controller_task() {
            controller_data data{m_status, m_predicate_result, std::move(m_predicate), std::move(m_stop_token)};

            awaiter_with_wait_hook hook_task{m_condition_variable, m_lock, data};
            m_condition_variable.push_waiter(static_cast<awaiter_base *>(&hook_task));

            static_cast<void>(m_lock.owned_mutex()->unlock());

            co_await silicon::coroutine::when_all(make_on_notify_callback_task(data), make_timeout_task(data));
            co_return;
        }

        bool await_ready() noexcept {

            if(!m_predicate.has_value()) {
                return false;
            }

            m_predicate_result = m_predicate.value()();
            if(m_predicate_result && m_stop_token.has_value() && m_stop_token.value().stop_requested()) {
                m_status = std::cv_status::no_timeout;
            }
            return m_predicate_result;
        }

        bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
            m_awaiting_coroutine = awaiting_coroutine;

            auto controller = make_controller_task();
            controller.resume();
            return true;
        }

        return_type await_resume() noexcept {
            if constexpr(std::is_same_v<return_type, bool>) {
                return m_predicate_result;
            } else {
                return m_status.value();
            }
        }

        silicon::scheduler::task<notify_status_t> do_on_notify() { std::unreachable(); }

        std::unique_ptr<io_executor_type> &m_executor;

        const std::chrono::nanoseconds m_wait_for;

        std::optional<std::cv_status> m_status{std::nullopt};

        bool m_predicate_result{false};

        std::optional<std::function<bool()>> m_predicate{std::nullopt};

        std::optional<const std::stop_token> m_stop_token{std::nullopt};
    };

#endif

  public:
    condition_variable();
    ~condition_variable();

    condition_variable(const condition_variable &) = delete;
    condition_variable(condition_variable &&) = delete;
    condition_variable & operator=(const condition_variable &) = delete;
    condition_variable & operator=(condition_variable &&) = delete;

    silicon::scheduler::task<void> notify_one() ;

    template<silicon::scheduler::concepts::executor executor_type>
    void notify_one(std::unique_ptr<executor_type> &executor) {
        executor->spawn_detached(notify_one());
    }

    silicon::scheduler::task<void> notify_all() ;

    template<silicon::scheduler::concepts::executor executor_type>
    void notify_all(std::unique_ptr<executor_type> &executor) {
        auto *waiter = pop_all_waiters();

        while(waiter != nullptr) {

            awaiter_base *next = waiter->m_next;

            executor->spawn_detached(make_notify_all_executor_individual_task(waiter));
            waiter = next;
        }

        return;
    }

    [[nodiscard]] auto wait(silicon::coroutine::scoped_lock &) -> awaiter;

    [[nodiscard]] auto wait(silicon::coroutine::scoped_lock &, std::function<bool()>) -> awaiter_with_predicate;

#ifndef EMSCRIPTEN

    [[nodiscard]] auto wait(silicon::coroutine::scoped_lock &, std::stop_token, std::function<bool()>)
            -> awaiter_with_predicate_stop_token;
#endif

#ifdef LIBCORO_FEATURE_NETWORKING

    template<silicon::scheduler::concepts::io_executor io_executor_type, class rep_type, class period_type>
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

    template<silicon::scheduler::concepts::io_executor io_executor_type, class rep_type, class period_type>
    [[nodiscard]] auto wait_for(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            const std::chrono::duration<rep_type, period_type> wait_for,
            std::function<bool()> predicate
    ) -> awaiter_with_wait<io_executor_type, bool> {
        return awaiter_with_wait<io_executor_type, bool>{
                executor,
                *this,
                lock,
                std::chrono::duration_cast<std::chrono::nanoseconds>(wait_for),
                std::move(predicate)
        };
    }

    template<silicon::scheduler::concepts::io_executor io_executor_type, class rep_type, class period_type>
    [[nodiscard]] auto wait_for(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            std::stop_token stop_token,
            const std::chrono::duration<rep_type, period_type> wait_for,
            std::function<bool()> predicate
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

    template<silicon::scheduler::concepts::io_executor io_executor_type, class clock_type, class duration_type>
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

    template<silicon::scheduler::concepts::io_executor io_executor_type, class clock_type, class duration_type>
    auto wait_until(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            const std::chrono::time_point<clock_type, duration_type> wait_until_time,
            std::function<bool()> predicate
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

    template<silicon::scheduler::concepts::io_executor io_executor_type, class clock_type, class duration_type>
    auto wait_until(
            std::unique_ptr<io_executor_type> &executor,
            silicon::coroutine::scoped_lock &lock,
            std::stop_token stop_token,
            const std::chrono::time_point<clock_type, duration_type> wait_until_time,
            std::function<bool()> predicate
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

    struct impl;
    std::unique_ptr<impl> m_p;

    awaiter_base * pop_all_waiters() noexcept ;

    void push_waiter(awaiter_base *) noexcept ;

    silicon::scheduler::task<void> make_notify_all_executor_individual_task(awaiter_base *) ;
};

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic pop
#endif

}
