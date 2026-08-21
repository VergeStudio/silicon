module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <memory>
#include <exception>



#include <atomic>
#include <coroutine>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

#include "silicon/coroutine/common.h"

export module silicon.coroutine:when_all;

import silicon.scheduler;
import :void_value;

export namespace silicon::coroutine {

class when_all_latch {
  public:
    when_all_latch(std::size_t) noexcept;

    when_all_latch(const when_all_latch &) = delete;
    when_all_latch(when_all_latch &&other);

    ~when_all_latch();

    when_all_latch & operator=(const when_all_latch &) = delete;
    when_all_latch & operator=(when_all_latch &&other) ;

    bool is_ready() const noexcept ;

    bool try_await(std::coroutine_handle<>) noexcept ;

    void notify_awaitable_completed() noexcept ;

  private:
    /// Implementation state, fully hidden in the implementation unit.
    struct impl;
    std::unique_ptr<impl> m_p;
};

template<typename task_container_type>
class when_all_ready_awaitable;

template<typename return_type>
class when_all_task;

/// Empty tuple<> implementation.
template<>
class when_all_ready_awaitable<std::tuple<>> {
  public:
    constexpr when_all_ready_awaitable() noexcept {}
    explicit constexpr when_all_ready_awaitable(std::tuple<>) noexcept {}

    constexpr bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    std::tuple<> await_resume() const noexcept { return {}; }
};

template<typename... task_types>
class when_all_ready_awaitable<std::tuple<task_types...>> {
  public:
    // 注意：impl 必须一次性聚合初始化。when_all_latch 无默认构造，
    // when_all_task<T> 的移动赋值亦为 delete，故不可"先默认构造再逐成员赋值"。
    explicit when_all_ready_awaitable(task_types &&...tasks) noexcept(
            std::conjunction<std::is_nothrow_move_constructible<task_types>...>::value
    )
        : m_p(new impl{when_all_latch{sizeof...(task_types)}, std::tuple<task_types...>{std::move(tasks)...}}) {}

    explicit when_all_ready_awaitable(std::tuple<task_types...> &&tasks) noexcept(
            std::is_nothrow_move_constructible_v<std::tuple<task_types...>>
    )
        : m_p(new impl{when_all_latch{sizeof...(task_types)}, std::move(tasks)}) {}

    when_all_ready_awaitable(const when_all_ready_awaitable &) = delete;
    // PIMPL 语义下移动即转移实现指针，无需逐成员移动。
    when_all_ready_awaitable(when_all_ready_awaitable &&other) noexcept: m_p(std::move(other.m_p)) {}

    when_all_ready_awaitable & operator=(const when_all_ready_awaitable &) = delete;
    when_all_ready_awaitable & operator=(when_all_ready_awaitable &&) = delete;

    auto operator co_await() & noexcept {
        struct awaiter {
            explicit awaiter(when_all_ready_awaitable &awaitable) noexcept: m_awaitable(awaitable) {}

            bool await_ready() const noexcept { return m_awaitable.is_ready(); }

            bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
                return m_awaitable.try_await(awaiting_coroutine);
            }

            std::tuple<task_types...> & await_resume() noexcept { return m_awaitable.m_p->m_tasks; }

          private:
            when_all_ready_awaitable &m_awaitable;
        };

        return awaiter{*this};
    }

    auto operator co_await() && noexcept {
        struct awaiter {
            explicit awaiter(when_all_ready_awaitable &awaitable) noexcept: m_awaitable(awaitable) {}

            bool await_ready() const noexcept { return m_awaitable.is_ready(); }

            bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
                return m_awaitable.try_await(awaiting_coroutine);
            }

            std::tuple<task_types...> && await_resume() noexcept { return std::move(m_awaitable.m_p->m_tasks); }

          private:
            when_all_ready_awaitable &m_awaitable;
        };

        return awaiter{*this};
    }

  private:
    bool is_ready() const noexcept { return m_p->m_latch.is_ready(); }

    bool try_await(std::coroutine_handle<> awaiting_coroutine) noexcept {
        std::apply([this](auto &&...tasks) { ((tasks.start(m_p->m_latch)), ...); }, m_p->m_tasks);
        return m_p->m_latch.try_await(awaiting_coroutine);
    }

    struct impl {
      public:
        when_all_latch m_latch;
        std::tuple<task_types...> m_tasks;
    };
    std::unique_ptr<impl> m_p;
};

template<typename task_container_type>
class when_all_ready_awaitable {
  public:
    // 同上：聚合初始化，且 std::size(tasks) 在移动 tasks 之前按序求值。
    explicit when_all_ready_awaitable(task_container_type &&tasks) noexcept
        : m_p(new impl{when_all_latch{std::size(tasks)}, std::forward<task_container_type>(tasks)}) {}

    when_all_ready_awaitable(const when_all_ready_awaitable &) = delete;
    when_all_ready_awaitable(when_all_ready_awaitable &&other) noexcept: m_p(std::move(other.m_p)) {}

    when_all_ready_awaitable & operator=(const when_all_ready_awaitable &) = delete;
    when_all_ready_awaitable & operator=(when_all_ready_awaitable &&) = delete;

    auto operator co_await() & noexcept {
        struct awaiter {
            awaiter(when_all_ready_awaitable &awaitable): m_awaitable(awaitable) {}

            bool await_ready() const noexcept { return m_awaitable.is_ready(); }

            bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
                return m_awaitable.try_await(awaiting_coroutine);
            }

            task_container_type & await_resume() noexcept { return m_awaitable.m_p->m_tasks; }

          private:
            when_all_ready_awaitable &m_awaitable;
        };

        return awaiter{*this};
    }

    auto operator co_await() && noexcept {
        struct awaiter {
            awaiter(when_all_ready_awaitable &awaitable): m_awaitable(awaitable) {}

            bool await_ready() const noexcept { return m_awaitable.is_ready(); }

            bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
                return m_awaitable.try_await(awaiting_coroutine);
            }

            task_container_type && await_resume() noexcept { return std::move(m_awaitable.m_p->m_tasks); }

          private:
            when_all_ready_awaitable &m_awaitable;
        };

        return awaiter{*this};
    }

  private:
    bool is_ready() const noexcept { return m_p->m_latch.is_ready(); }

    bool try_await(std::coroutine_handle<> awaiting_coroutine) noexcept {
        for(auto &task: m_p->m_tasks) {
            task.start(m_p->m_latch);
        }

        return m_p->m_latch.try_await(awaiting_coroutine);
    }

    struct impl {
      public:
        when_all_latch m_latch;
        task_container_type m_tasks;
    };
    std::unique_ptr<impl> m_p;
};

template<typename return_type>
class when_all_task_promise {
  public:
    using coroutine_handle_type = std::coroutine_handle<when_all_task_promise<return_type>>;

    when_all_task_promise() noexcept {}

    auto get_return_object() noexcept { return coroutine_handle_type::from_promise(*this); }

    std::suspend_always initial_suspend() noexcept { return {}; }

    auto final_suspend() noexcept {
        struct completion_notifier {
            bool await_ready() const noexcept { return false; }
            void await_suspend(coroutine_handle_type coroutine) const noexcept {
                coroutine.promise().m_p->m_latch->notify_awaitable_completed();
            }
            auto await_resume() const noexcept {}
        };

        return completion_notifier{};
    }

    auto unhandled_exception() noexcept { m_p->m_exception_ptr = std::current_exception(); }

    auto yield_value(return_type &&value) noexcept {
        m_p->m_return_value = std::addressof(value);
        return final_suspend();
    }

    void start(when_all_latch &latch) noexcept {
        m_p->m_latch = &latch;
        coroutine_handle_type::from_promise(*this).resume();
    }

    return_type & result() & {
        if(m_p->m_exception_ptr) {
            std::rethrow_exception(m_p->m_exception_ptr);
        }
        return *m_p->m_return_value;
    }

    const return_type & result() const & {
        if(m_p->m_exception_ptr) {
            std::rethrow_exception(m_p->m_exception_ptr);
        }
        return *m_p->m_return_value;
    }

    return_type && result() && {
        if(m_p->m_exception_ptr) {
            std::rethrow_exception(m_p->m_exception_ptr);
        }
        return std::move(*m_p->m_return_value);
    }

    void return_void() noexcept {
        // We should have either suspended at co_yield point or
        // an exception was thrown before running off the end of
        // the coroutine.
        std::unreachable();
    }

  private:
    struct impl {
      public:
        when_all_latch *m_latch{nullptr};
        std::exception_ptr m_exception_ptr;
        std::add_pointer_t<return_type> m_return_value;
    };
    std::unique_ptr<impl> m_p{std::make_unique<impl>()};
};

template<>
class when_all_task_promise<void> {
  public:
    using coroutine_handle_type = std::coroutine_handle<when_all_task_promise<void>>;

    when_all_task_promise() noexcept {}

    auto get_return_object() noexcept { return coroutine_handle_type::from_promise(*this); }

    std::suspend_always initial_suspend() noexcept { return {}; }

    auto final_suspend() noexcept {
        struct completion_notifier {
            bool await_ready() const noexcept { return false; }
            void await_suspend(coroutine_handle_type coroutine) const noexcept {
                coroutine.promise().m_p->m_latch->notify_awaitable_completed();
            }
            void await_resume() const noexcept {}
        };

        return completion_notifier{};
    }

    void unhandled_exception() noexcept { m_p->m_exception_ptr = std::current_exception(); }

    void return_void() noexcept {}

    void result() {
        if(m_p->m_exception_ptr) {
            std::rethrow_exception(m_p->m_exception_ptr);
        }
    }

    void start(when_all_latch &latch) {
        m_p->m_latch = &latch;
        coroutine_handle_type::from_promise(*this).resume();
    }

  private:
    struct impl {
      public:
        when_all_latch *m_latch{nullptr};
        std::exception_ptr m_exception_ptr;
    };
    std::unique_ptr<impl> m_p{std::make_unique<impl>()};
};

template<typename return_type>
class when_all_task {
  public:
    // To be able to call start().
    template<typename task_container_type>
    friend class when_all_ready_awaitable;

    using promise_type = when_all_task_promise<return_type>;
    using coroutine_handle_type = typename promise_type::coroutine_handle_type;

    when_all_task(coroutine_handle_type coroutine) noexcept: m_p(std::make_unique<impl>()) { m_p->m_coroutine = coroutine; }

    when_all_task(const when_all_task &) = delete;
    when_all_task(when_all_task &&other) noexcept
        : m_p(std::make_unique<impl>()) {
        m_p->m_coroutine = std::exchange(other.m_p->m_coroutine, coroutine_handle_type{});
    }

    when_all_task & operator=(const when_all_task &) = delete;
    when_all_task & operator=(when_all_task &&) = delete;

    ~when_all_task() {
        if(m_p->m_coroutine != nullptr) {
            m_p->m_coroutine.destroy();
        }
    }

    return_type & return_value() & {
        return m_p->m_coroutine.promise().result();
    }

    const return_type & return_value() const & {
        return m_p->m_coroutine.promise().result();
    }

    return_type && return_value() && {
        return std::move(m_p->m_coroutine.promise()).result();
    }

  private:
    void start(when_all_latch &latch) noexcept { m_p->m_coroutine.promise().start(latch); }

    struct impl {
      public:
        coroutine_handle_type m_coroutine{};
    };
    std::unique_ptr<impl> m_p;
};

template<>
class when_all_task<void> {
  public:
    // To be able to call start().
    template<typename task_container_type>
    friend class when_all_ready_awaitable;

    using promise_type = when_all_task_promise<void>;
    using coroutine_handle_type = typename promise_type::coroutine_handle_type;

    when_all_task(coroutine_handle_type coroutine) noexcept: m_p(std::make_unique<impl>()) { m_p->m_coroutine = coroutine; }

    when_all_task(const when_all_task &) = delete;
    when_all_task(when_all_task &&other) noexcept
        : m_p(std::make_unique<impl>()) {
        m_p->m_coroutine = std::exchange(other.m_p->m_coroutine, coroutine_handle_type{});
    }

    when_all_task & operator=(const when_all_task &) = delete;
    when_all_task & operator=(when_all_task &&) = delete;

    ~when_all_task() {
        if(m_p->m_coroutine != nullptr) {
            m_p->m_coroutine.destroy();
        }
    }

    void_value return_value() {
        m_p->m_coroutine.promise().result();
        return void_value{};
    }

  private:
    void start(when_all_latch &latch) noexcept { m_p->m_coroutine.promise().start(latch); }

    struct impl {
      public:
        coroutine_handle_type m_coroutine{};
    };
    std::unique_ptr<impl> m_p;
};

template<
        concepts::awaitable awaitable,
        typename return_type = typename concepts::awaitable_traits<awaitable &&>::awaiter_return_type>
when_all_task<return_type> __ATTRIBUTE__(used) make_when_all_task(awaitable) ;

template<concepts::awaitable awaitable, typename return_type>
when_all_task<return_type> make_when_all_task(awaitable a) {
    if constexpr(std::is_void_v<return_type>) {
        co_await static_cast<awaitable &&>(a);
        co_return;
    } else {
        co_yield co_await static_cast<awaitable &&>(a);
    }
}



template<concepts::awaitable... awaitables_type>
[[nodiscard]] auto when_all(awaitables_type... awaitables) {
    return when_all_ready_awaitable<std::tuple<
            when_all_task<typename concepts::awaitable_traits<awaitables_type>::awaiter_return_type>...>>(
            std::make_tuple(make_when_all_task(std::move(awaitables))...)
    );
}

template<
        std::ranges::range range_type,
        concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>,
        typename return_type = typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type>
[[nodiscard]] auto when_all(range_type awaitables)
        -> when_all_ready_awaitable<std::vector<when_all_task<return_type>>> {
    std::vector<when_all_task<return_type>> output_tasks;

    // If the size is known in constant time reserve the output tasks size.
    if constexpr(std::ranges::sized_range<range_type>) {
        output_tasks.reserve(std::size(awaitables));
    }

    // Wrap each task into a when_all_task.
    for(auto &&a: awaitables) {
        output_tasks.emplace_back(make_when_all_task(std::move(a)));
    }

    // Return the single awaitable that drives all the user's tasks.
    return when_all_ready_awaitable(std::move(output_tasks));
}

} // namespace silicon::coroutine
