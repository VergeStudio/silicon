module;

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <cstdint>
#include <deque>
#include <exception>
#include <expected>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include <map>
#include <optional>

module silicon.scheduler;

import :poll_info_impl;

namespace silicon::scheduler {


static silicon::scheduler::task<void> make_spawned_joinable_wait_task(std::unique_ptr<silicon::scheduler::task_group<silicon::scheduler::thread_pool>> group_ptr) {
    co_await *group_ptr;
    co_return;
}


struct thread_pool::impl {
    options m_opts;

    struct alignas(64) ThreadState {
        std::deque<std::coroutine_handle<void>> queue;

        mutable std::mutex mutex;
    };


    std::deque<ThreadState> m_states;

    std::vector<std::thread> m_threads;

    std::atomic<std::size_t> m_submit_idx{0};

    std::mutex m_wait_mutex;
    std::condition_variable_any m_wait_cv;

    std::atomic<std::size_t> m_size{0};
    std::atomic<bool> m_shutdown_requested{false};

    void executor(std::size_t) ;
    void schedule_impl(std::coroutine_handle<void>) noexcept ;

    static std::coroutine_handle<void> pop_front(ThreadState &state) noexcept {
        std::scoped_lock lk{state.mutex};
        if(state.queue.empty()) return nullptr;
        auto h = state.queue.front();
        state.queue.pop_front();
        return h;
    }

    static std::coroutine_handle<void> steal_back(ThreadState &state) noexcept {
        std::scoped_lock lk{state.mutex};
        if(state.queue.empty()) return nullptr;
        auto h = state.queue.back();
        state.queue.pop_back();
        return h;
    }

    bool all_queues_empty() const noexcept {
        if(m_size.load(std::memory_order::acquire) == 0) return true;
        for(auto &state: m_states) {
            std::scoped_lock lk{state.mutex};
            if(!state.queue.empty()) return false;
        }
        return true;
    }

    static std::size_t steal_start(std::size_t my_idx, std::size_t) {
        return my_idx + 1;
    }
};

thread_pool::schedule_operation::schedule_operation(thread_pool &tp) noexcept: m_thread_pool(tp) {}

void thread_pool::schedule_operation::await_suspend(std::coroutine_handle<void> awaiting_coroutine) noexcept {
    m_thread_pool.m_impl->schedule_impl(awaiting_coroutine);
}

thread_pool::thread_pool(options &&opts, private_constructor): m_impl(std::make_unique<impl>()) {
    m_impl->m_opts = std::move(opts);
    auto n = m_impl->m_opts.thread_count;
    m_impl->m_threads.reserve(n);
    m_impl->m_states.resize(n);
}

std::expected<std::unique_ptr<thread_pool>, std::error_code> thread_pool::create(options opts) {
    auto tp = std::make_unique<thread_pool>(std::move(opts), private_constructor{});
    auto &impl = *tp->m_impl;
    try {
        for(uint32_t i = 0; i < impl.m_opts.thread_count; ++i) {
            impl.m_threads.emplace_back([tp = tp.get(), i]() { tp->m_impl->executor(i); });
        }
    } catch(const std::exception &) {

        return std::unexpected(make_error_code(scheduler_error::kUnknown));
    }
    return tp;
}

thread_pool::~thread_pool() { shutdown(); }

auto thread_pool::thread_count() const noexcept -> size_t { return m_impl->m_threads.size(); }

auto thread_pool::schedule() -> schedule_operation {
    m_impl->m_size.fetch_add(1, std::memory_order::release);
    if(!m_impl->m_shutdown_requested.load(std::memory_order::acquire)) {
        return schedule_operation{*this};
    } else {


        m_impl->m_size.fetch_sub(1, std::memory_order::release);
        std::terminate();
    }
}

bool thread_pool::spawn_detached(silicon::scheduler::task<void> &&task) noexcept {
    m_impl->m_size.fetch_add(1, std::memory_order::release);
    auto wrapper_task = silicon::scheduler::make_task_self_deleting(std::move(task));
    wrapper_task.promise().user_final_suspend([impl = m_impl.get()]() -> void { impl->m_size.fetch_sub(1, std::memory_order::release); });
    return resume(wrapper_task.handle());
}

silicon::scheduler::task<void> thread_pool::spawn_joinable(silicon::scheduler::task<void> &&task) noexcept {
    auto group_ptr = std::make_unique<silicon::scheduler::task_group<silicon::scheduler::thread_pool>>(this, std::move(task));
    return make_spawned_joinable_wait_task(std::move(group_ptr));
}

bool thread_pool::resume(std::coroutine_handle<void> handle) noexcept {
    if(handle == nullptr || handle.done()) return false;
    m_impl->m_size.fetch_add(1, std::memory_order::release);
    if(m_impl->m_shutdown_requested.load(std::memory_order::acquire)) {
        m_impl->m_size.fetch_sub(1, std::memory_order::release);
        return false;
    }
    m_impl->schedule_impl(handle);
    return true;
}

void thread_pool::shutdown() noexcept {
    auto &impl = *m_impl;
    if(impl.m_shutdown_requested.exchange(true, std::memory_order::acq_rel) == false) {
        {
            std::unique_lock<std::mutex> lk{impl.m_wait_mutex};
            impl.m_wait_cv.notify_all();
        }
        for(auto &thread: impl.m_threads) {
            if(thread.joinable()) thread.join();
        }
    }
}

bool thread_pool::is_shutdown() const {
    return m_impl->m_shutdown_requested.load(std::memory_order::acquire);
}

std::size_t thread_pool::size() const noexcept {
    return m_impl->m_size.load(std::memory_order::acquire);
}

std::size_t thread_pool::queue_size() const noexcept {
    std::size_t total = 0;
    for(auto &state: m_impl->m_states) {
        std::scoped_lock lk{state.mutex};
        total += state.queue.size();
    }
    return total;
}

void thread_pool::impl::executor(std::size_t idx) {
    if(m_opts.on_thread_start_functor != nullptr) {
        m_opts.on_thread_start_functor(idx);
    }

    auto &my_state = m_states[idx];
    const auto n = m_states.size();
    const auto start_victim = steal_start(idx, n);

    while(!m_shutdown_requested.load(std::memory_order::acquire)) {
        auto handle = pop_front(my_state);
        if(handle) {
            handle.resume();
            m_size.fetch_sub(1, std::memory_order::release);
            continue;
        }

        for(std::size_t attempt = 0; attempt < n; ++attempt) {
            auto victim = (start_victim + attempt) % n;
            if(victim == idx) continue;
            handle = steal_back(m_states[victim]);
            if(handle) break;
        }
        if(handle) {
            handle.resume();
            m_size.fetch_sub(1, std::memory_order::release);
            continue;
        }

        std::unique_lock<std::mutex> lk{m_wait_mutex};
        m_wait_cv.wait(lk, [&]() {
            return !all_queues_empty() || m_shutdown_requested.load(std::memory_order::acquire);
        });
    }

    while(m_size.load(std::memory_order::acquire) > 0) {
        auto handle = pop_front(my_state);
        if(!handle) {
            for(std::size_t attempt = 0; attempt < n; ++attempt) {
                auto victim = (start_victim + attempt) % n;
                if(victim == idx) continue;
                handle = steal_back(m_states[victim]);
                if(handle) break;
            }
        }
        if(!handle) break;
        handle.resume();
        m_size.fetch_sub(1, std::memory_order::release);
    }

    if(m_opts.on_thread_stop_functor != nullptr) {
        m_opts.on_thread_stop_functor(idx);
    }
}

void thread_pool::impl::schedule_impl(std::coroutine_handle<void> handle) noexcept {
    if(handle == nullptr || handle.done()) return;

    auto idx = m_submit_idx.fetch_add(1, std::memory_order::relaxed) % m_states.size();
    auto &state = m_states[idx];
    {
        std::scoped_lock lk{state.mutex};
        state.queue.emplace_back(handle);
    }
    m_wait_cv.notify_one();
}

std::size_t thread_pool::resume_range_impl(std::vector<std::coroutine_handle<void>> &handles) noexcept {
    auto &impl = *m_impl;
    impl.m_size.fetch_add(handles.size(), std::memory_order::release);

    std::size_t null_handles{0};
    const auto n = impl.m_states.size();

    for(std::size_t i = 0; i < handles.size(); ++i) {
        auto &h = handles[i];
        if(h != nullptr) [[likely]] {
            auto &state = impl.m_states[i % n];
            std::scoped_lock lk{state.mutex};
            state.queue.emplace_back(h);
        } else {
            ++null_handles;
        }
    }

    if(null_handles > 0) {
        impl.m_size.fetch_sub(null_handles, std::memory_order::release);
    }

    std::size_t total = handles.size() - null_handles;
    if(total >= n) {
        impl.m_wait_cv.notify_all();
    } else {
        for(uint64_t i = 0; i < total; ++i) {
            impl.m_wait_cv.notify_one();
        }
    }
    return total;
}

}
