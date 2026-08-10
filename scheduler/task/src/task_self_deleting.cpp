module;

#include <atomic>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

module silicon.scheduler.task;

namespace silicon::scheduler {

promise_self_deleting::promise_self_deleting(promise_self_deleting &&other) noexcept
    : m_user_final_suspend(std::exchange(other.m_user_final_suspend, nullptr)) {
}

auto promise_self_deleting::operator=(promise_self_deleting &&other) noexcept -> promise_self_deleting & {
    if(std::addressof(other) != this) {
        m_user_final_suspend = std::exchange(other.m_user_final_suspend, nullptr);
    }
    return *this;
}

auto promise_self_deleting::get_return_object() -> task_self_deleting {
    return task_self_deleting{*this};
}

auto promise_self_deleting::initial_suspend() -> std::suspend_always {
    return std::suspend_always{};
}

auto promise_self_deleting::final_suspend() noexcept -> std::suspend_never {
    if(m_user_final_suspend != nullptr) {
        m_user_final_suspend();
    }
    return std::suspend_never{};
}

auto promise_self_deleting::return_void() noexcept -> void {}

auto promise_self_deleting::unhandled_exception() -> void {}

auto promise_self_deleting::user_final_suspend(std::function<void()> user_final_suspend) noexcept -> void {
    m_user_final_suspend = std::move(user_final_suspend);
}

task_self_deleting::task_self_deleting(promise_self_deleting &promise): m_promise(&promise) {}
task_self_deleting::task_self_deleting(task_self_deleting &&other) noexcept: m_promise(other.m_promise) {}

auto task_self_deleting::operator=(task_self_deleting &&other) noexcept -> task_self_deleting & {
    if(std::addressof(other) != this) {
        m_promise = other.m_promise;
    }
    return *this;
}

auto task_self_deleting::handle() const -> std::coroutine_handle<promise_self_deleting> {
    return std::coroutine_handle<promise_self_deleting>::from_promise(*m_promise);
}

auto task_self_deleting::handle() -> std::coroutine_handle<promise_self_deleting> {
    return std::coroutine_handle<promise_self_deleting>::from_promise(*m_promise);
}

auto make_task_self_deleting(silicon::scheduler::task<void> user_task) -> task_self_deleting {
    co_await user_task;
    co_return;
}

} // namespace silicon::scheduler
