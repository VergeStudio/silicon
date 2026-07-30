module;

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

module silicon.thread;

namespace silicon::thread {

namespace detail {
static auto
make_spawned_joinable_wait_task(std::unique_ptr<silicon::task::task_group<silicon::thread::pool>> group_ptr) -> silicon::task::task<void> {
    co_await *group_ptr;
    co_return;
}
} // namespace detail

struct pool::Impl {
    options m_opts;

    struct alignas(64) ThreadState {
        std::deque<std::coroutine_handle<void>> queue;
        // mutable：all_queues_empty() 等 const 成员需在只读语义下加锁。
        mutable std::mutex mutex;
    };
    // ThreadState 含 std::mutex（不可移动），vector::resize 要求 move-insertable，
    // 属非法实例化；deque::resize 原位构造且不搬迁元素，故改用 deque。
    std::deque<ThreadState> m_states;

    std::vector<std::thread> m_threads;

    std::atomic<std::size_t> m_submit_idx{0};

    std::mutex m_wait_mutex;
    std::condition_variable_any m_wait_cv;

    std::atomic<std::size_t> m_size{0};
    std::atomic<bool> m_shutdown_requested{false};

    auto executor(std::size_t idx) -> void;
    auto schedule_impl(std::coroutine_handle<void> handle) noexcept -> void;

    static auto pop_front(ThreadState &state) noexcept -> std::coroutine_handle<void> {
        std::scoped_lock lk{state.mutex};
        if(state.queue.empty()) return nullptr;
        auto h = state.queue.front();
        state.queue.pop_front();
        return h;
    }

    static auto steal_back(ThreadState &state) noexcept -> std::coroutine_handle<void> {
        std::scoped_lock lk{state.mutex};
        if(state.queue.empty()) return nullptr;
        auto h = state.queue.back();
        state.queue.pop_back();
        return h;
    }

    auto all_queues_empty() const noexcept -> bool {
        if(m_size.load(std::memory_order::acquire) == 0) return true;
        for(auto &state: m_states) {
            std::scoped_lock lk{state.mutex};
            if(!state.queue.empty()) return false;
        }
        return true;
    }

    static auto steal_start(std::size_t my_idx, std::size_t count) -> std::size_t {
        return my_idx + 1;
    }
};

pool::schedule_operation::schedule_operation(pool &tp) noexcept: m_thread_pool(tp) {}

auto pool::schedule_operation::await_suspend(std::coroutine_handle<void> awaiting_coroutine) noexcept -> void {
    m_thread_pool.m_impl->schedule_impl(awaiting_coroutine);
}

pool::pool(options &&opts, private_constructor): m_impl(std::make_unique<Impl>()) {
    m_impl->m_opts = std::move(opts);
    auto n = m_impl->m_opts.thread_count;
    m_impl->m_threads.reserve(n);
    m_impl->m_states.resize(n);
}

auto pool::make_unique(options opts) -> std::unique_ptr<pool> {
    auto tp = std::make_unique<pool>(std::move(opts), private_constructor{});
    auto &impl = *tp->m_impl;
    for(uint32_t i = 0; i < impl.m_opts.thread_count; ++i) {
        impl.m_threads.emplace_back([tp = tp.get(), i]() { tp->m_impl->executor(i); });
    }
    return tp;
}

pool::~pool() { shutdown(); }

auto pool::thread_count() const noexcept -> size_t { return m_impl->m_threads.size(); }

auto pool::schedule() -> schedule_operation {
    m_impl->m_size.fetch_add(1, std::memory_order::release);
    if(!m_impl->m_shutdown_requested.load(std::memory_order::acquire)) {
        return schedule_operation{*this};
    } else {
        m_impl->m_size.fetch_sub(1, std::memory_order::release);
        throw std::runtime_error("silicon::thread::pool is shutting down, unable to schedule new tasks.");
    }
}

auto pool::spawn_detached(silicon::task::task<void> &&task) noexcept -> bool {
    m_impl->m_size.fetch_add(1, std::memory_order::release);
    auto wrapper_task = silicon::task::detail::make_task_self_deleting(std::move(task));
    wrapper_task.promise().user_final_suspend([impl = m_impl.get()]() -> void { impl->m_size.fetch_sub(1, std::memory_order::release); });
    return resume(wrapper_task.handle());
}

auto pool::spawn_joinable(silicon::task::task<void> &&task) noexcept -> silicon::task::task<void> {
    auto group_ptr = std::make_unique<silicon::task::task_group<silicon::thread::pool>>(this, std::move(task));
    return detail::make_spawned_joinable_wait_task(std::move(group_ptr));
}

auto pool::resume(std::coroutine_handle<void> handle) noexcept -> bool {
    if(handle == nullptr || handle.done()) return false;
    m_impl->m_size.fetch_add(1, std::memory_order::release);
    if(m_impl->m_shutdown_requested.load(std::memory_order::acquire)) {
        m_impl->m_size.fetch_sub(1, std::memory_order::release);
        return false;
    }
    m_impl->schedule_impl(handle);
    return true;
}

auto pool::shutdown() noexcept -> void {
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

auto pool::is_shutdown() const -> bool {
    return m_impl->m_shutdown_requested.load(std::memory_order::acquire);
}

auto pool::size() const noexcept -> std::size_t {
    return m_impl->m_size.load(std::memory_order::acquire);
}

auto pool::queue_size() const noexcept -> std::size_t {
    std::size_t total = 0;
    for(auto &state: m_impl->m_states) {
        std::scoped_lock lk{state.mutex};
        total += state.queue.size();
    }
    return total;
}

auto pool::Impl::executor(std::size_t idx) -> void {
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

auto pool::Impl::schedule_impl(std::coroutine_handle<void> handle) noexcept -> void {
    if(handle == nullptr || handle.done()) return;

    auto idx = m_submit_idx.fetch_add(1, std::memory_order::relaxed) % m_states.size();
    auto &state = m_states[idx];
    {
        std::scoped_lock lk{state.mutex};
        state.queue.emplace_back(handle);
    }
    m_wait_cv.notify_one();
}

auto pool::resume_range_impl(std::vector<std::coroutine_handle<void>> &handles) noexcept -> std::size_t {
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

} // namespace silicon::thread
