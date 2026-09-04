module;

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <utility>
#include <map>
#include <optional>

module silicon.scheduler;

import :poll_info_impl;

namespace silicon::scheduler {




static auto make_spawned_joinable_wait_task(std::unique_ptr<task_group<run_loop>> group_ptr) -> task<void> {
    co_await *group_ptr;
    co_return;
}


struct run_loop::impl {
    std::mutex m_mutex{};
    std::condition_variable m_cv{};
    std::deque<std::coroutine_handle<>> m_queue{};
    std::atomic<bool> m_stop{false};
    std::atomic<std::size_t> m_size{0};
};

run_loop::run_loop(): m_impl(std::make_unique<impl>()) {}

run_loop::~run_loop() {


    finish();
}

void run_loop::run() noexcept {
    auto &impl = *m_impl;
    for(;;) {
        std::coroutine_handle<> handle;
        {
            std::unique_lock lk{impl.m_mutex};
            impl.m_cv.wait(lk, [&]() {
                return !impl.m_queue.empty() || impl.m_stop.load(std::memory_order::acquire);
            });
            if(impl.m_queue.empty()) {

                break;
            }
            handle = impl.m_queue.front();
            impl.m_queue.pop_front();
        }
        handle.resume();
        impl.m_size.fetch_sub(1, std::memory_order::release);
    }
}

void run_loop::finish() noexcept {
    auto &impl = *m_impl;
    if(impl.m_stop.exchange(true, std::memory_order::acq_rel) == false) {
        std::unique_lock lk{impl.m_mutex};
        impl.m_cv.notify_all();
    }
}

bool run_loop::resume(std::coroutine_handle<> handle) noexcept {
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

bool run_loop::spawn_detached(task<void> &&task) noexcept {
    auto &impl = *m_impl;



    impl.m_size.fetch_add(1, std::memory_order::release);
    auto wrapper = make_task_self_deleting(std::move(task));
    wrapper.promise().user_final_suspend([impl = m_impl.get()]() -> void {
        impl->m_size.fetch_sub(1, std::memory_order::release);
    });
    return resume(wrapper.handle());
}

auto run_loop::spawn_joinable(task<void> &&t) noexcept -> task<void> {
    auto group_ptr = std::make_unique<task_group<run_loop>>(this, std::move(t));
    return make_spawned_joinable_wait_task(std::move(group_ptr));
}

void run_loop::shutdown() noexcept {
    finish();
}

bool run_loop::is_shutdown() const {
    return m_impl->m_stop.load(std::memory_order::acquire);
}

std::size_t run_loop::size() const noexcept {
    return m_impl->m_size.load(std::memory_order::acquire);
}

}
