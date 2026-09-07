module;

#include <memory>
#include <utility>

#include <coroutine>

#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <variant>

#include "silicon/common.h"

export module silicon.scheduler:sync_wait;

import :concepts.awaitable;

export namespace silicon::scheduler {

struct SILICON_CORE_API unset_return_value {
    unset_return_value() {}
    unset_return_value(unset_return_value &&) = delete;
    unset_return_value(const unset_return_value &) = delete;
    auto operator=(unset_return_value &&) = delete;
    auto operator=(const unset_return_value &) = delete;
};

class SILICON_CORE_API sync_wait_event {
  public:
    sync_wait_event(bool = false);
    sync_wait_event(const sync_wait_event &) = delete;
    sync_wait_event(sync_wait_event &&) = delete;
    sync_wait_event & operator=(const sync_wait_event &) = delete;
    sync_wait_event & operator=(sync_wait_event &&) = delete;
    ~sync_wait_event();

    void set() noexcept ;
    void reset() noexcept ;
    void wait() noexcept ;

  private:
    struct impl;
    std::unique_ptr<impl> m_p;
};

class SILICON_CORE_API sync_wait_task_promise_base {
  public:
    sync_wait_task_promise_base() noexcept = default;

    std::suspend_always initial_suspend() noexcept { return {}; }

  protected:
    virtual ~sync_wait_task_promise_base() = default;
};

template<typename return_type>
class SILICON_CORE_API sync_wait_task_promise: public sync_wait_task_promise_base {
  public:
    using coroutine_type = std::coroutine_handle<sync_wait_task_promise<return_type>>;

    static constexpr bool return_type_is_reference = std::is_reference_v<return_type>;
    using stored_type = std::conditional_t<
            return_type_is_reference,
            std::remove_reference_t<return_type> *,
            std::remove_const_t<return_type>>;
    using variant_type = std::variant<unset_return_value, stored_type, std::exception_ptr>;

    sync_wait_task_promise() noexcept = default;
    sync_wait_task_promise(const sync_wait_task_promise &) = delete;
    sync_wait_task_promise(sync_wait_task_promise &&) = delete;
    sync_wait_task_promise & operator=(const sync_wait_task_promise &) = delete;
    sync_wait_task_promise & operator=(sync_wait_task_promise &&) = delete;
    ~sync_wait_task_promise() override = default;

    auto start(sync_wait_event &event) {
        m_p->m_event = &event;
        coroutine_type::from_promise(*this).resume();
    }

    auto get_return_object() noexcept { return coroutine_type::from_promise(*this); }

    template<typename value_type>
        requires(return_type_is_reference and std::is_constructible_v<return_type, value_type &&>) or
                (not return_type_is_reference and std::is_constructible_v<stored_type, value_type &&>)
    void return_value(value_type &&value) {
        if constexpr(return_type_is_reference) {
            return_type ref = static_cast<value_type &&>(value);
            m_p->m_storage.template emplace<stored_type>(std::addressof(ref));
        } else {
            m_p->m_storage.template emplace<stored_type>(std::forward<value_type>(value));
        }
    }

    auto return_value(stored_type value) -> void
        requires(not return_type_is_reference)
    {
        if constexpr(std::is_move_constructible_v<stored_type>) {
            m_p->m_storage.template emplace<stored_type>(std::move(value));
        } else {
            m_p->m_storage.template emplace<stored_type>(value);
        }
    }

    void unhandled_exception() noexcept {
        m_p->m_storage.template emplace<std::exception_ptr>(std::current_exception());
    }

    auto final_suspend() noexcept {
        struct completion_notifier {
            auto await_ready() const noexcept { return false; }
            auto await_suspend(coroutine_type coroutine) const noexcept { coroutine.promise().m_p->m_event->set(); }
            auto await_resume() noexcept {};
        };

        return completion_notifier{};
    }

    decltype(auto) result() & {
        if(std::holds_alternative<stored_type>(m_p->m_storage)) {
            if constexpr(return_type_is_reference) {
                return static_cast<return_type>(*std::get<stored_type>(m_p->m_storage));
            } else {
                return static_cast<const return_type &>(std::get<stored_type>(m_p->m_storage));
            }
        } else if(std::holds_alternative<std::exception_ptr>(m_p->m_storage)) {
            std::rethrow_exception(std::get<std::exception_ptr>(m_p->m_storage));
        } else {

            std::terminate();
        }
    }

    decltype(auto) result() const & {
        if(std::holds_alternative<stored_type>(m_p->m_storage)) {
            if constexpr(return_type_is_reference) {
                return static_cast<std::add_const_t<return_type>>(*std::get<stored_type>(m_p->m_storage));
            } else {
                return static_cast<const return_type &>(std::get<stored_type>(m_p->m_storage));
            }
        } else if(std::holds_alternative<std::exception_ptr>(m_p->m_storage)) {
            std::rethrow_exception(std::get<std::exception_ptr>(m_p->m_storage));
        } else {
            std::terminate();
        }
    }

    decltype(auto) result() && {
        if(std::holds_alternative<stored_type>(m_p->m_storage)) {
            if constexpr(return_type_is_reference) {
                return static_cast<return_type>(*std::get<stored_type>(m_p->m_storage));
            } else if constexpr(std::is_constructible_v<return_type, stored_type>) {
                return static_cast<return_type &&>(std::get<stored_type>(m_p->m_storage));
            } else {
                return static_cast<const return_type &&>(std::get<stored_type>(m_p->m_storage));
            }
        } else if(std::holds_alternative<std::exception_ptr>(m_p->m_storage)) {
            std::rethrow_exception(std::get<std::exception_ptr>(m_p->m_storage));
        } else {
            std::terminate();
        }
    }

  private:
    struct impl {
      public:
        sync_wait_event *m_event{nullptr};
        variant_type m_storage{};
    };
    std::unique_ptr<impl> m_p{std::make_unique<impl>()};
};

template<>
class SILICON_CORE_API sync_wait_task_promise<void>: public sync_wait_task_promise_base {
    using coroutine_type = std::coroutine_handle<sync_wait_task_promise<void>>;

  public:
    sync_wait_task_promise() noexcept = default;
    ~sync_wait_task_promise() override = default;

    auto start(sync_wait_event &event) {
        m_p->m_event = &event;
        coroutine_type::from_promise(*this).resume();
    }

    auto get_return_object() noexcept { return coroutine_type::from_promise(*this); }

    struct SILICON_CORE_API completion_notifier {
        auto await_ready() const noexcept { return false; }
        auto await_suspend(coroutine_type coroutine) const noexcept { coroutine.promise().m_p->m_event->set(); }
        auto await_resume() noexcept {};
    };

    auto final_suspend() noexcept { return completion_notifier{}; }

    void unhandled_exception() { m_p->m_exception = std::current_exception(); }

    void return_void() noexcept {}

    void result() {
        if(m_p->m_exception) {
            std::rethrow_exception(m_p->m_exception);
        }
    }

  private:
    struct impl {
      public:
        sync_wait_event *m_event{nullptr};
        std::exception_ptr m_exception;
    };
    std::unique_ptr<impl> m_p{std::make_unique<impl>()};
};

template<typename return_type>
class sync_wait_task {
  public:
    using promise_type = sync_wait_task_promise<return_type>;
    using coroutine_type = std::coroutine_handle<promise_type>;

    sync_wait_task(coroutine_type coroutine) noexcept: m_p(std::make_unique<impl>()) { m_p->m_coroutine = coroutine; }

    sync_wait_task(const sync_wait_task &) = delete;
    sync_wait_task(sync_wait_task &&other) noexcept: m_p(std::make_unique<impl>()) { m_p->m_coroutine = std::exchange(other.m_p->m_coroutine, coroutine_type{}); }
    sync_wait_task & operator=(const sync_wait_task &) = delete;
    sync_wait_task & operator=(sync_wait_task &&other) {
        if(std::addressof(other) != this) {
            m_p->m_coroutine = std::exchange(other.m_p->m_coroutine, coroutine_type{});
        }

        return *this;
    }

    ~sync_wait_task() {
        if(m_p->m_coroutine) {
            m_p->m_coroutine.destroy();
        }
    }

    promise_type & promise() & { return m_p->m_coroutine.promise(); }
    const promise_type & promise() const & { return m_p->m_coroutine.promise(); }
    promise_type && promise() && { return std::move(m_p->m_coroutine.promise()); }

  private:
    struct impl {
      public:
        coroutine_type m_coroutine{};
    };
    std::unique_ptr<impl> m_p;
};

template<
        concepts::awaitable awaitable_type,
        typename return_type = concepts::awaitable_traits<awaitable_type>::awaiter_return_type>
sync_wait_task<return_type> __ATTRIBUTE__(used) make_sync_wait_task(awaitable_type &&a) ;

template<concepts::awaitable awaitable_type, typename return_type>
sync_wait_task<return_type> make_sync_wait_task(awaitable_type &&a) {
    if constexpr(std::is_void_v<return_type>) {
        co_await std::forward<awaitable_type>(a);
        co_return;
    } else {
        co_return co_await std::forward<awaitable_type>(a);
    }
}

template<
        concepts::awaitable awaitable_type,
        typename return_type = typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type>
return_type sync_wait(awaitable_type &&a) {
    sync_wait_event e{};
    auto task = make_sync_wait_task(std::forward<awaitable_type>(a));
    task.promise().start(e);
    e.wait();

    if constexpr(std::is_void_v<return_type>) {
        task.promise().result();
        return;
    } else if constexpr(std::is_reference_v<return_type>) {
        return task.promise().result();
    } else if constexpr(std::is_move_constructible_v<std::remove_const_t<return_type>>) {
        return std::move(task).promise().result();
    } else {
        return task.promise().result();
    }
}

}
