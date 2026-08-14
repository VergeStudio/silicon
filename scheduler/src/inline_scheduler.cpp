module;

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <memory>
#include <utility>

module silicon.scheduler;

namespace silicon::scheduler {

// 与 run_loop / thread_pool 一致的便捷封装：把 user_task 包成自删除任务，并返回其等待任务，
// 使 spawn_joinable 的返回句柄在整组任务完成时变为 ready。

static auto make_spawned_joinable_wait_task(std::unique_ptr<task::task_group<inline_scheduler>> group_ptr) -> task::task<void> {
    co_await *group_ptr;
    co_return;
}


struct inline_scheduler::impl {
    std::atomic<bool> m_stop{false};
    std::atomic<std::size_t> m_size{0};
};

inline_scheduler::inline_scheduler(): m_impl(std::make_unique<impl>()) {}

inline_scheduler::~inline_scheduler() {
    // 内联调度器不拥有线程，shutdown() 仅置停止标志，无需等待任何线程退出。
    shutdown();
}

auto inline_scheduler::spawn_detached(task::task<void> &&task) noexcept -> bool {
    auto &impl = *m_impl;
    if(impl.m_stop.load(std::memory_order::acquire)) {
        return false;
    }
    // 与 thread_pool::spawn_detached 一致的计数 / 所有权语义：
    //   spawn 计数 +1，由自删除任务完成时经 user_final_suspend 计数 -1；
    //   resume 计数 +1，由本函数内 resume() 返回前计数 -1（内联执行，无队列）。
    impl.m_size.fetch_add(1, std::memory_order::release);
    auto wrapper = task::make_task_self_deleting(std::move(task));
    wrapper.promise().user_final_suspend([impl = m_impl.get()]() -> void {
        impl->m_size.fetch_sub(1, std::memory_order::release);
    });
    return resume(wrapper.handle());
}

auto inline_scheduler::resume(std::coroutine_handle<> handle) noexcept -> bool {
    if(handle == nullptr || handle.done()) {
        return false;
    }
    auto &impl = *m_impl;
    if(impl.m_stop.load(std::memory_order::acquire)) {
        return false;
    }
    // 内联执行：在当前线程就地 resume()，执行完毕（或再次挂起）后再减计数。
    impl.m_size.fetch_add(1, std::memory_order::release);
    handle.resume();
    impl.m_size.fetch_sub(1, std::memory_order::release);
    return true;
}

auto inline_scheduler::spawn_joinable(task::task<void> &&task) noexcept -> task::task<void> {
    auto group_ptr = std::make_unique<task::task_group<inline_scheduler>>(this, std::move(task));
    return make_spawned_joinable_wait_task(std::move(group_ptr));
}

auto inline_scheduler::shutdown() noexcept -> void {
    m_impl->m_stop.store(true, std::memory_order::release);
}

auto inline_scheduler::is_shutdown() const -> bool {
    return m_impl->m_stop.load(std::memory_order::acquire);
}

auto inline_scheduler::size() const noexcept -> std::size_t {
    return m_impl->m_size.load(std::memory_order::acquire);
}

} // namespace silicon::scheduler
