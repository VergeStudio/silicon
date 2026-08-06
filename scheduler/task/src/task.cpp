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

module silicon.scheduler.task;

namespace silicon::scheduler::task {

namespace detail {

auto promise_base::final_awaitable::await_ready() const noexcept -> bool {
    return false;
}

auto promise_base::final_awaitable::await_resume() noexcept -> void {}

auto promise_base::continuation(std::coroutine_handle<> continuation) noexcept -> void {
    m_continuation = continuation;
}

[[nodiscard]] auto task_self_deleting::promise() const -> const promise_self_deleting & {
    return *m_promise;
}

[[nodiscard]] auto task_self_deleting::promise() -> promise_self_deleting & {
    return *m_promise;
}

} // namespace detail

task_event::awaiter::awaiter(const task_event &e) noexcept: m_event(e) {}

auto task_event::awaiter::await_ready() const noexcept -> bool {
    return m_event.is_set();
}

auto task_event::awaiter::await_resume() noexcept -> void {}

} // namespace silicon::scheduler::task
