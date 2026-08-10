module;

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <utility>

module silicon.scheduler;

namespace silicon::scheduler {

// 与 thread_pool 一致的便捷封装：把 user_task 包成自删除任务，并返回其等待任务，
// 使 spawn_joinable 的返回句柄在整组任务完成时变为 ready。

static auto make_spawned_joinable_wait_task(std::unique_ptr<task::task_group<run_loop>> group_ptr) -> task::task<void> {
    co_await *group_ptr;
    co_return;
}


struct run_loop::Impl {
    std::mutex m_mutex{};
    std::condition_variable m_cv{};
    std::deque<std::coroutine_handle<>> m_queue{};
    std::atomic<bool> m_stop{false};
    std::atomic<std::size_t> m_size{0};
};

run_loop::run_loop(): m_impl(std::make_unique<Impl>()) {}

run_loop::~run_loop() {
    // 仅唤醒可能阻塞在空队列上的 run()；不等待 run() 所在的外部线程退出
    // （run_loop 不持有线程，无法 join）。承载 run() 的线程须比本对象更晚销毁。
    finish();
}

auto run_loop::run() noexcept -> void {
    auto &impl = *m_impl;
    for(;;) {
        std::coroutine_handle<> handle;
        {
            std::unique_lock lk{impl.m_mutex};
            impl.m_cv.wait(lk, [&]() {
                return !impl.m_queue.empty() || impl.m_stop.load(std::memory_order::acquire);
            });
            if(impl.m_queue.empty()) {
                // 仅因 m_stop 置位而唤醒、且队列已空 → 退出循环。
                break;
            }
            handle = impl.m_queue.front();
            impl.m_queue.pop_front();
        }
        handle.resume();
        impl.m_size.fetch_sub(1, std::memory_order::release);
    }
}

auto run_loop::finish() noexcept -> void {
    auto &impl = *m_impl;
    if(impl.m_stop.exchange(true, std::memory_order::acq_rel) == false) {
        std::unique_lock lk{impl.m_mutex};
        impl.m_cv.notify_all();
    }
}

auto run_loop::resume(std::coroutine_handle<> handle) noexcept -> bool {
    if(handle == nullptr || handle.done()) {
        return false;
    }
    auto &impl = *m_impl;
    if(impl.m_stop.load(std::memory_order::acquire)) {
        return false;
    }
    impl.m_size.fetch_add(1, std::memory_order::release);
    {
        std::scoped_lock lk{impl.m_mutex};
        impl.m_queue.emplace_back(handle);
    }
    impl.m_cv.notify_one();
    return true;
}

auto run_loop::spawn_detached(task::task<void> &&task) noexcept -> bool {
    auto &impl = *m_impl;
    // 与 thread_pool::spawn_detached 相同的计数/所有权语义：
    //   spawn 计数 +1，由自删除任务完成时经 user_final_suspend 计数 -1；
    //   resume 计数 +1，由 run() 在 resume() 后计数 -1。
    impl.m_size.fetch_add(1, std::memory_order::release);
    auto wrapper = task::make_task_self_deleting(std::move(task));
    wrapper.promise().user_final_suspend([impl = m_impl.get()]() -> void {
        impl->m_size.fetch_sub(1, std::memory_order::release);
    });
    return resume(wrapper.handle());
}

auto run_loop::spawn_joinable(task::task<void> &&task) noexcept -> task::task<void> {
    auto group_ptr = std::make_unique<task::task_group<run_loop>>(this, std::move(task));
    return make_spawned_joinable_wait_task(std::move(group_ptr));
}

auto run_loop::shutdown() noexcept -> void {
    finish();
}

auto run_loop::is_shutdown() const -> bool {
    return m_impl->m_stop.load(std::memory_order::acquire);
}

auto run_loop::size() const noexcept -> std::size_t {
    return m_impl->m_size.load(std::memory_order::acquire);
}

} // namespace silicon::scheduler
