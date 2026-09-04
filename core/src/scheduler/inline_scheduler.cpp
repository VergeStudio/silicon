module;

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <memory>
#include <utility>
#include <map>
#include <optional>

module silicon.scheduler;

import :poll_info_impl;

namespace silicon::scheduler {

static auto make_spawned_joinable_wait_task(std::unique_ptr<task_group<inline_scheduler>> group_ptr) -> task<void> {
    co_await *group_ptr;
    co_return;
}

struct inline_scheduler::impl {
    std::atomic<bool> m_stop{false};
    std::atomic<std::size_t> m_size{0};
};

inline_scheduler::inline_scheduler(): m_impl(std::make_unique<impl>()) {}

inline_scheduler::~inline_scheduler() {

    shutdown();
}

bool inline_scheduler::spawn_detached(task<void> &&task) noexcept {
    auto &impl = *m_impl;
    if(impl.m_stop.load(std::memory_order::acquire)) {
        return false;
    }

    impl.m_size.fetch_add(1, std::memory_order::release);
    auto wrapper = make_task_self_deleting(std::move(task));
    wrapper.promise().user_final_suspend([impl = m_impl.get()]() -> void {
        impl->m_size.fetch_sub(1, std::memory_order::release);
    });
    return resume(wrapper.handle());
}

bool inline_scheduler::resume(std::coroutine_handle<> handle) noexcept {
    if(handle == nullptr || handle.done()) {
        return false;
    }
    auto &impl = *m_impl;
    if(impl.m_stop.load(std::memory_order::acquire)) {
        return false;
    }

    impl.m_size.fetch_add(1, std::memory_order::release);
    handle.resume();
    impl.m_size.fetch_sub(1, std::memory_order::release);
    return true;
}

auto inline_scheduler::spawn_joinable(task<void> &&t) noexcept -> task<void> {
    auto group_ptr = std::make_unique<task_group<inline_scheduler>>(this, std::move(t));
    return make_spawned_joinable_wait_task(std::move(group_ptr));
}

void inline_scheduler::shutdown() noexcept {
    m_impl->m_stop.store(true, std::memory_order::release);
}

bool inline_scheduler::is_shutdown() const {
    return m_impl->m_stop.load(std::memory_order::acquire);
}

std::size_t inline_scheduler::size() const noexcept {
    return m_impl->m_size.load(std::memory_order::acquire);
}

}
