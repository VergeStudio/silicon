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

export module silicon.scheduler.task;

export import :config;

export namespace silicon::scheduler {

// task 的前置声明必须先于 promise —— promise 的 get_return_object()
// 以 task<return_type> 为返回类型，而 task 的完整定义在其后。默认实参 = void
// 亦仅可在此首次声明处给出，供 task<> 与 promise<void> 使用。
template<typename return_type = void>
class task;



struct promise_base {
    friend struct final_awaitable;
    struct final_awaitable {
        auto await_ready() const noexcept -> bool;

        template<typename promise_type>
        auto await_suspend(std::coroutine_handle<promise_type> coroutine) noexcept -> std::coroutine_handle<> {
            auto &promise = coroutine.promise();
            if(promise.m_continuation != nullptr) {
                return promise.m_continuation;
            } else {
                return std::noop_coroutine();
            }
        }

        auto await_resume() noexcept -> void;
    };

    promise_base() noexcept = default;
    ~promise_base() = default;

    // inline-defined so the deduced return types are visible to other modules
    // that co_await a task across the module boundary (clang needs the return
    // type of initial_suspend/final_suspend at the co_await site).
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    auto final_suspend() noexcept { return final_awaitable{}; }
    auto continuation(std::coroutine_handle<> continuation) noexcept -> void;

  protected:
    std::coroutine_handle<> m_continuation{nullptr};
};

template<typename return_type>
struct promise final: public promise_base {
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

    auto get_return_object() noexcept -> task_t;

    template<typename value_type>
        requires(return_type_is_reference and std::is_constructible_v<return_type, value_type &&>) or
                (not return_type_is_reference and
                 std::is_constructible_v<stored_type, value_type &&>)
    auto return_value(value_type &&value) -> void {
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

    auto unhandled_exception() noexcept -> void {
        m_storage.template emplace<std::exception_ptr>(std::current_exception());
    }

    auto result() & -> decltype(auto) {
        if(std::holds_alternative<stored_type>(m_storage)) {
            if constexpr(return_type_is_reference) {
                return static_cast<return_type>(*std::get<stored_type>(m_storage));
            } else {
                return static_cast<const return_type &>(std::get<stored_type>(m_storage));
            }
        } else if(std::holds_alternative<std::exception_ptr>(m_storage)) {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        } else {
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

    auto result() const & -> decltype(auto) {
        if(std::holds_alternative<stored_type>(m_storage)) {
            if constexpr(return_type_is_reference) {
                return static_cast<std::add_const_t<return_type>>(*std::get<stored_type>(m_storage));
            } else {
                return static_cast<const return_type &>(std::get<stored_type>(m_storage));
            }
        } else if(std::holds_alternative<std::exception_ptr>(m_storage)) {
            std::rethrow_exception(std::get<std::exception_ptr>(m_storage));
        } else {
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

    auto result() && -> decltype(auto) {
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
            throw std::runtime_error{"The return value was never set, did you execute the coroutine?"};
        }
    }

  private:
    variant_type m_storage{};
};

template<>
struct promise<void>: public promise_base {
    using task_t = task<void>;
    using coroutine_handle = std::coroutine_handle<promise<void>>;

    promise() noexcept = default;
    promise(const promise &) = delete;
    promise(promise &&other) = delete;
    promise &operator=(const promise &) = delete;
    promise &operator=(promise &&other) = delete;
    ~promise() = default;

    auto get_return_object() noexcept -> task_t;

    auto return_void() noexcept -> void {}

    auto unhandled_exception() noexcept -> void { m_exception_ptr = std::current_exception(); }

    auto result() -> void {
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
        auto await_ready() const noexcept -> bool { return !m_coroutine || m_coroutine.done(); }
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> std::coroutine_handle<> {
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

    auto operator=(const task &) -> task & = delete;
    auto operator=(task &&other) noexcept -> task & {
        if(std::addressof(other) != this) {
            if(m_coroutine != nullptr) {
                m_coroutine.destroy();
            }
            m_coroutine = std::exchange(other.m_coroutine, nullptr);
        }
        return *this;
    }

    auto is_ready() const noexcept -> bool { return m_coroutine == nullptr || m_coroutine.done(); }

    auto resume() -> bool {
        if(!m_coroutine.done()) {
            m_coroutine.resume();
        }
        return !m_coroutine.done();
    }

    auto destroy() -> bool {
        if(m_coroutine != nullptr) {
            m_coroutine.destroy();
            m_coroutine = nullptr;
            return true;
        }
        return false;
    }

    auto operator co_await() const & noexcept {
        struct awaitable: public awaitable_base {
            auto await_resume() -> decltype(auto) { return this->m_coroutine.promise().result(); }
        };
        return awaitable{m_coroutine};
    }

    auto operator co_await() const && noexcept {
        struct awaitable: public awaitable_base {
            auto await_resume() -> decltype(auto) { return std::move(this->m_coroutine.promise()).result(); }
        };
        return awaitable{m_coroutine};
    }

    auto promise() & -> promise_type & { return m_coroutine.promise(); }
    auto promise() const & -> const promise_type & { return m_coroutine.promise(); }
    auto promise() && -> promise_type && { return std::move(m_coroutine.promise()); }
    auto handle() -> coroutine_handle { return m_coroutine; }

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



// ---------------------------------------------------------------------------
// task_self_deleting — a coroutine that self-destructs upon completion
// ---------------------------------------------------------------------------
//
// NOTE: 这些内部辅助类型此前由公共头 include/silicon/scheduler/task/detail/
// task_self_deleting.hpp 提供；该兼容头已删除，本模块接口现为唯一定义处。
// silicon::scheduler::task 现指类模板本身，不能作命名空间限定符；
// 辅助类型（task_self_deleting / make_task_self_deleting）位于
// silicon::scheduler 命名空间（detail 层已消除），实现单元
// src/task_self_deleting.cpp 使用同一命名空间，修饰名一致。
// 消费方统一经 `import silicon.scheduler.task;` 使用，不再走头文件路径。


class task_self_deleting;

class promise_self_deleting {
  public:
    promise_self_deleting() = default;
    ~promise_self_deleting() = default;

    promise_self_deleting(const promise_self_deleting &) = delete;
    promise_self_deleting(promise_self_deleting &&) noexcept;
    auto operator=(const promise_self_deleting &) -> promise_self_deleting & = delete;
    auto operator=(promise_self_deleting &&) noexcept -> promise_self_deleting &;

    auto get_return_object() -> task_self_deleting;
    auto initial_suspend() -> std::suspend_always;
    auto final_suspend() noexcept -> std::suspend_never;
    auto return_void() noexcept -> void;
    auto unhandled_exception() -> void;
    auto user_final_suspend(std::function<void()> user_final_suspend) noexcept -> void;

  private:
    std::function<void()> m_user_final_suspend{nullptr};
};

class task_self_deleting {
  public:
    using promise_type = promise_self_deleting;
    explicit task_self_deleting(promise_self_deleting &promise);
    ~task_self_deleting() = default;

    task_self_deleting(const task_self_deleting &) = delete;
    task_self_deleting(task_self_deleting &&) noexcept;
    auto operator=(const task_self_deleting &) -> task_self_deleting & = delete;
    auto operator=(task_self_deleting &&) noexcept -> task_self_deleting &;

    [[nodiscard]] auto promise() const -> const promise_self_deleting &;
    [[nodiscard]] auto promise() -> promise_self_deleting &;
    [[nodiscard]] auto handle() const -> std::coroutine_handle<promise_self_deleting>;
    [[nodiscard]] auto handle() -> std::coroutine_handle<promise_self_deleting>;
    auto resume() -> bool;

  private:
    promise_self_deleting *m_promise{nullptr};
};

auto make_task_self_deleting(silicon::scheduler::task<void> user_task) -> task_self_deleting;



// ---------------------------------------------------------------------------
// task_event — minimal coroutine-aware event for task_group
// ---------------------------------------------------------------------------

class task_event {
  public:
    struct awaiter {
        awaiter(const task_event &e) noexcept;
        auto await_ready() const noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
        auto await_resume() noexcept -> void;

        const task_event &m_event;
        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        awaiter *m_next{nullptr};
    };

    explicit task_event(bool initially_set = false) noexcept;
    ~task_event() = default;
    task_event(const task_event &) = delete;
    task_event(task_event &&) = delete;
    auto operator=(const task_event &) -> task_event & = delete;
    auto operator=(task_event &&) -> task_event & = delete;

    auto is_set() const noexcept -> bool;
    auto set() noexcept -> void;
    auto reset() noexcept -> void;
    auto operator co_await() const noexcept -> awaiter;

  private:
    friend struct awaiter;
    mutable std::atomic<void *> m_state{nullptr};
};

// ---------------------------------------------------------------------------
// task_group — manage a group of related tasks on an executor
// ---------------------------------------------------------------------------

template<typename executor_type>
class task_group {
  public:
    explicit task_group(executor_type *executor)
        : m_executor(executor) {
        if(executor == nullptr) {
            throw std::runtime_error{"task_group cannot have a nullptr executor"};
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
    auto operator=(const task_group &) -> task_group & = delete;
    auto operator=(task_group &&) -> task_group & = delete;

    ~task_group() {
        // Spin-wait with yield instead of sleep_for to minimize latency.
        // Note: This destructor blocks until all tasks complete. Do not call
        // from within a coroutine running on the same executor — prefer
        // co_await *this before destruction instead.
        while(!empty()) {
            std::this_thread::yield();
        }
    }

    [[nodiscard]] auto start(silicon::scheduler::task<void> &&task) -> bool {
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

    [[nodiscard]] auto size() const -> std::size_t { return m_size.load(std::memory_order::acquire); }
    [[nodiscard]] auto empty() const -> bool { return size() == 0; }
    auto operator co_await() const noexcept -> task_event::awaiter { return m_on_empty_event.operator co_await(); }

  private:
    executor_type *m_executor{nullptr};
    std::atomic<uint64_t> m_size{};
    task_event m_on_empty_event{true};

    auto count_down() -> void {
        if(m_size.fetch_sub(1, std::memory_order::acq_rel) == 1) {
            m_on_empty_event.set();
        }
    }
};

} // namespace silicon::scheduler
