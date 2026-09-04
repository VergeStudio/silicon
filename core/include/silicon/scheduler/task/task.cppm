module;

#include <atomic>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <variant>

#include "silicon/common.h"

export module silicon.scheduler.task;

export namespace silicon::scheduler {

template<typename return_type = void>
class task;

struct CORE_API promise_base {
    friend struct final_awaitable;
    struct CORE_API final_awaitable {
        bool await_ready() const noexcept ;

        template<typename promise_type>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept {
            auto &promise = coroutine.promise();
            if(promise.m_continuation != nullptr) {
                return promise.m_continuation;
            } else {
                return std::noop_coroutine();
            }
        }

        void await_resume() noexcept ;
    };

    promise_base() noexcept = default;
    ~promise_base() = default;

    auto initial_suspend() noexcept { return std::suspend_always{}; }
    auto final_suspend() noexcept { return final_awaitable{}; }

    void continuation(std::coroutine_handle<> continuation) noexcept { m_continuation = continuation; }

  protected:
    std::coroutine_handle<> m_continuation{nullptr};
};

template<typename return_type>
struct CORE_API promise final: public promise_base {
  private:
    struct unset_return_value {
        unset_return_value() {}
        unset_return_value(unset_return_value &&) = delete;
        unset_return_value(const unset_return_value &) = delete;
        auto operator=(unset_return_value &&) = delete;
        auto operator=(const unset_return_value &) = delete;
    };

  public:
    using task_t = task<return_type>;
    using coroutine_handle = std::coroutine_handle<promise<return_type>>;
    static constexpr bool return_type_is_reference = std::is_reference_v<return_type>;
    using stored_type = std::conditional_t<
            return_type_is_reference,
            std::remove_reference_t<return_type> *,
            std::remove_const_t<return_type>>;
    using variant_type = std::variant<unset_return_value, stored_type, std::exception_ptr>;

    promise() noexcept {}
    promise(const promise &) = delete;
    promise(promise &&other) = delete;
    promise &operator=(const promise &) = delete;
    promise &operator=(promise &&other) = delete;
    ~promise() = default;

    task_t get_return_object() noexcept ;

    template<typename value_type>
        requires(return_type_is_reference and std::is_constructible_v<return_type, value_type &&>) or
                (not return_type_is_reference and
                 std::is_constructible_v<stored_type, value_type &&>)
    void return_value(value_type &&value) {
        if constexpr(return_type_is_reference) {
            return_type ref = static_cast<value_type &&>(value);
            m_storage.template emplace<stored_type>(std::addressof(ref));
        } else {
            m_storage.template emplace<stored_type>(std::forward<value_type>(value));
        }
    }

    auto return_value(stored_type &&value) -> void
        requires(not return_type_is_reference)
    {
        if constexpr(std::is_move_constructible_v<stored_type>) {
            m_storage.template emplace<stored_type>(std::move(value));
        } else {
            m_storage.template emplace<stored_type>(value);
        }
    }

    void unhandled_exception() noexcept {
        m_storage.template emplace<std::exception_ptr>(std::current_exception());
    }

    decltype(auto) result() & {
        if(std::holds_alternative<stored_type>(m_storage)) {
            if constexpr(return_type_is_reference) {
                return static_cast<return_type>(*std::get<stored_type>(m_storage));
            } else {
                return static_cast<const return_type &>(std::get<stored_type>(m_storage));
            }
        } else if(std::holds_alternative<std::exception_ptr>(m_storage)) {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        } else {

            std::terminate();
        }
    }

    decltype(auto) result() const & {
        if(std::holds_alternative<stored_type>(m_storage)) {
            if constexpr(return_type_is_reference) {
                return static_cast<std::add_const_t<return_type>>(*std::get<stored_type>(m_storage));
            } else {
                return static_cast<const return_type &>(std::get<stored_type>(m_storage));
            }
        } else if(std::holds_alternative<std::exception_ptr>(m_storage)) {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        } else {
            std::terminate();
        }
    }

    decltype(auto) result() && {
        if(std::holds_alternative<stored_type>(m_storage)) {
            if constexpr(return_type_is_reference) {
                return static_cast<return_type>(*std::get<stored_type>(m_storage));
            } else if constexpr(std::is_move_constructible_v<return_type>) {
                return static_cast<return_type &&>(std::get<stored_type>(m_storage));
            } else {
                return static_cast<const return_type &&>(std::get<stored_type>(m_storage));
            }
        } else if(std::holds_alternative<std::exception_ptr>(m_storage)) {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        } else {
            std::terminate();
        }
    }

  private:
    variant_type m_storage{};
};

template<>
struct CORE_API promise<void>: public promise_base {
    using task_t = task<void>;
    using coroutine_handle = std::coroutine_handle<promise<void>>;

    promise() noexcept = default;
    promise(const promise &) = delete;
    promise(promise &&other) = delete;
    promise &operator=(const promise &) = delete;
    promise &operator=(promise &&other) = delete;
    ~promise() = default;

    task_t get_return_object() noexcept ;

    void return_void() noexcept {}

    void unhandled_exception() noexcept { m_exception_ptr = std::current_exception(); }

    void result() {
        if(m_exception_ptr) {
            std::rethrow_exception(m_exception_ptr);
        }
    }

  private:
    std::exception_ptr m_exception_ptr{nullptr};
};

template<typename return_type>
class [[nodiscard]] task {
  public:
    using task_t = task<return_type>;
    using promise_type = promise<return_type>;
    using coroutine_handle = std::coroutine_handle<promise_type>;

    struct awaitable_base {
        awaitable_base(coroutine_handle coroutine) noexcept: m_coroutine(coroutine) {}
        bool await_ready() const noexcept { return !m_coroutine || m_coroutine.done(); }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
            m_coroutine.promise().continuation(awaiting_coroutine);
            return m_coroutine;
        }
        std::coroutine_handle<promise_type> m_coroutine{nullptr};
    };

    task() noexcept: m_coroutine(nullptr) {}
    explicit task(coroutine_handle handle): m_coroutine(handle) {}
    task(const task &) = delete;
    task(task &&other) noexcept: m_coroutine(std::exchange(other.m_coroutine, nullptr)) {}

    ~task() {
        if(m_coroutine != nullptr) {
            m_coroutine.destroy();
        }
    }

    task & operator=(const task &) = delete;
    task & operator=(task &&other) noexcept {
        if(std::addressof(other) != this) {
            if(m_coroutine != nullptr) {
                m_coroutine.destroy();
            }
            m_coroutine = std::exchange(other.m_coroutine, nullptr);
        }
        return *this;
    }

    bool is_ready() const noexcept { return m_coroutine == nullptr || m_coroutine.done(); }

    bool resume() {
        if(!m_coroutine.done()) {
            m_coroutine.resume();
        }
        return !m_coroutine.done();
    }

    bool destroy() {
        if(m_coroutine != nullptr) {
            m_coroutine.destroy();
            m_coroutine = nullptr;
            return true;
        }
        return false;
    }

    auto operator co_await() const & noexcept {
        struct awaitable: public awaitable_base {
            decltype(auto) await_resume() { return this->m_coroutine.promise().result(); }
        };
        return awaitable{m_coroutine};
    }

    auto operator co_await() const && noexcept {
        struct awaitable: public awaitable_base {
            decltype(auto) await_resume() { return std::move(this->m_coroutine.promise()).result(); }
        };
        return awaitable{m_coroutine};
    }

    promise_type & promise() & { return m_coroutine.promise(); }
    const promise_type & promise() const & { return m_coroutine.promise(); }
    promise_type && promise() && { return std::move(m_coroutine.promise()); }
    coroutine_handle handle() { return m_coroutine; }

  private:
    coroutine_handle m_coroutine{nullptr};
};

template<typename return_type>
inline auto promise<return_type>::get_return_object() noexcept -> task<return_type> {
    return task<return_type>{coroutine_handle::from_promise(*this)};
}

inline auto promise<void>::get_return_object() noexcept -> task<> {
    return task<>{coroutine_handle::from_promise(*this)};
}

class task_self_deleting;

class CORE_API promise_self_deleting {
  public:
    promise_self_deleting() = default;
    ~promise_self_deleting() = default;

    promise_self_deleting(const promise_self_deleting &) = delete;
    promise_self_deleting(promise_self_deleting &&) noexcept;
    promise_self_deleting & operator=(const promise_self_deleting &) = delete;
    promise_self_deleting & operator=(promise_self_deleting &&) noexcept ;

    task_self_deleting get_return_object() ;
    std::suspend_always initial_suspend() ;
    std::suspend_never final_suspend() noexcept ;
    void return_void() noexcept ;
    void unhandled_exception() ;
    void user_final_suspend(std::function<void()>) noexcept ;

  private:
    std::function<void()> m_user_final_suspend{nullptr};
};

class CORE_API task_self_deleting {
  public:
    using promise_type = promise_self_deleting;
    explicit task_self_deleting(promise_self_deleting &);
    ~task_self_deleting() = default;

    task_self_deleting(const task_self_deleting &) = delete;
    task_self_deleting(task_self_deleting &&) noexcept;
    task_self_deleting & operator=(const task_self_deleting &) = delete;
    task_self_deleting & operator=(task_self_deleting &&) noexcept ;

    [[nodiscard]] const promise_self_deleting & promise() const ;
    [[nodiscard]] promise_self_deleting & promise() ;
    [[nodiscard]] std::coroutine_handle<promise_self_deleting> handle() const ;
    [[nodiscard]] std::coroutine_handle<promise_self_deleting> handle() ;
    bool resume() ;

  private:
    promise_self_deleting *m_promise{nullptr};
};

CORE_API auto make_task_self_deleting(silicon::scheduler::task<void>) -> task_self_deleting;

class CORE_API task_event {
  public:

    struct CORE_API awaiter {
        awaiter(const task_event &) noexcept;
        bool await_ready() const noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;
        void await_resume() noexcept ;

        const task_event &m_event;
        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        awaiter *m_next{nullptr};
    };

    explicit task_event(bool = false) noexcept;
    ~task_event() = default;
    task_event(const task_event &) = delete;
    task_event(task_event &&) = delete;
    task_event & operator=(const task_event &) = delete;
    task_event & operator=(task_event &&) = delete;

    bool is_set() const noexcept ;
    void set() noexcept ;
    void reset() noexcept ;
    auto operator co_await() const noexcept -> awaiter;

  private:
    friend struct awaiter;
    mutable std::atomic<void *> m_state{nullptr};
};

template<typename executor_type>
class task_group {
  public:
    explicit task_group(executor_type *executor)
        : m_executor(executor) {

        if(executor == nullptr) {
            std::terminate();
        }
    }

    explicit task_group(executor_type *executor, silicon::scheduler::task<void> &&task)
        : task_group(executor) {
        (void)start(std::forward<silicon::scheduler::task<void>>(task));
    }

    template<typename range_type>
    explicit task_group(executor_type *executor, range_type tasks): task_group(executor) {
        for(auto &t: tasks) {
            (void)start(std::move(t));
        }
    }

    explicit task_group(std::unique_ptr<executor_type> &executor): m_executor(executor.get()) {}
    explicit task_group(std::unique_ptr<executor_type> &executor, silicon::scheduler::task<void> &&task)
        : task_group(executor.get(), std::forward<silicon::scheduler::task<void>>(task)) {}

    template<typename range_type>
    explicit task_group(std::unique_ptr<executor_type> &executor, range_type tasks)
        : task_group(executor.get(), std::forward<range_type>(tasks)) {}

    task_group(const task_group &) = delete;
    task_group(task_group &&) = delete;
    task_group & operator=(const task_group &) = delete;
    task_group & operator=(task_group &&) = delete;

    ~task_group() {

        while(!empty()) {
            std::this_thread::yield();
        }
    }

    [[nodiscard]] bool start(silicon::scheduler::task<void> &&task) {
        m_on_empty_event.reset();
        m_size.fetch_add(1, std::memory_order::release);
        auto wrapper_task = make_task_self_deleting(std::move(task));
        wrapper_task.promise().user_final_suspend([this]() -> void { count_down(); });
        if(!m_executor->resume(wrapper_task.handle())) {
            count_down();
            return false;
        }
        return true;
    }

    [[nodiscard]] std::size_t size() const { return m_size.load(std::memory_order::acquire); }
    [[nodiscard]] bool empty() const { return size() == 0; }
    auto operator co_await() const noexcept -> task_event::awaiter { return m_on_empty_event.operator co_await(); }

  private:
    executor_type *m_executor{nullptr};
    std::atomic<uint64_t> m_size{};
    task_event m_on_empty_event{true};

    void count_down() {
        if(m_size.fetch_sub(1, std::memory_order::acq_rel) == 1) {
            m_on_empty_event.set();
        }
    }
};

}
