#pragma once

// Minimal event primitive for use by task_group.
// This is a simplified version of silicon::coroutine::event without
// executor concept dependency, suitable for use in the standalone task library.

#include <atomic>
#include <coroutine>

namespace silicon::scheduler::task {

class task_event {
  public:
    struct awaiter {
        awaiter(const task_event &e) noexcept: m_event(e) {}
        auto await_ready() const noexcept -> bool { return m_event.is_set(); }
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
        auto await_resume() noexcept {}

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

} // namespace silicon::scheduler::task
