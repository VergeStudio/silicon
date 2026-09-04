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

namespace silicon::scheduler {

bool promise_base::final_awaitable::await_ready() const noexcept {
    return false;
}

void promise_base::final_awaitable::await_resume() noexcept {}

[[nodiscard]] auto task_self_deleting::promise() const -> const promise_self_deleting & {
    return *m_promise;
}

[[nodiscard]] auto task_self_deleting::promise() -> promise_self_deleting & {
    return *m_promise;
}

task_event::awaiter::awaiter(const task_event &e) noexcept: m_event(e) {}

bool task_event::awaiter::await_ready() const noexcept {
    return m_event.is_set();
}

void task_event::awaiter::await_resume() noexcept {}

}
