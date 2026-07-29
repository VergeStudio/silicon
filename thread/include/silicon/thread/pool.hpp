#pragma once

// NOTE: This file is kept for backward compatibility.
// The recommended way is via C++23 modules:
//   import silicon.thread;
// This header may be removed in a future version once all consumers
// have migrated to modules.

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <thread>
#include <variant>
#include <vector>

#include "silicon/task/task.hpp"
#include "silicon/task/task_group.hpp"

namespace silicon::thread {

class pool {
    struct private_constructor {
        explicit private_constructor() = default;
    };

  public:
    class schedule_operation {
        friend class pool;
        explicit schedule_operation(pool &tp) noexcept;

      public:
        auto await_ready() noexcept -> bool { return false; }
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> void;
        auto await_resume() noexcept -> void {}

      private:
        pool &m_thread_pool;
    };

    struct options {
        uint32_t thread_count = std::thread::hardware_concurrency();
        std::function<void(std::size_t)> on_thread_start_functor = nullptr;
        std::function<void(std::size_t)> on_thread_stop_functor = nullptr;
    };

    explicit pool(options &&opts, private_constructor);

    static auto make_unique(
            options opts = options{
                    .thread_count = std::thread::hardware_concurrency(),
                    .on_thread_start_functor = nullptr,
                    .on_thread_stop_functor = nullptr
            }
    ) -> std::unique_ptr<pool>;

    pool(const pool &) = delete;
    pool(pool &&) = delete;
    auto operator=(const pool &) -> pool & = delete;
    auto operator=(pool &&) -> pool & = delete;

    virtual ~pool();

    [[nodiscard]] auto thread_count() const noexcept -> size_t { return m_threads.size(); }
    [[nodiscard]] auto schedule() -> schedule_operation;
    auto spawn_detached(silicon::task::task<void> &&task) noexcept -> bool;
    auto spawn_joinable(silicon::task::task<void> &&task) noexcept -> silicon::task::task<void>;

    template<typename return_type>
    [[nodiscard]] auto schedule(silicon::task::task<return_type> task) -> silicon::task::task<return_type> {
        co_await schedule();
        co_return co_await task;
    }

    auto resume(std::coroutine_handle<> handle) noexcept -> bool;

    template<typename range_type>
    auto resume(const range_type &handles) noexcept -> std::size_t {
        m_size.fetch_add(std::size(handles), std::memory_order::release);
        std::size_t null_handles{0};
        {
            std::scoped_lock lk{m_wait_mutex};
            for(const auto &handle: handles) {
                if(handle != nullptr) [[likely]] {
                    m_queue.emplace_back(handle);
                } else {
                    ++null_handles;
                }
            }
        }
        if(null_handles > 0) {
            m_size.fetch_sub(null_handles, std::memory_order::release);
        }
        std::size_t total = std::size(handles) - null_handles;
        if(total >= m_threads.size()) {
            m_wait_cv.notify_all();
        } else {
            for(uint64_t i = 0; i < total; ++i) {
                m_wait_cv.notify_one();
            }
        }
        return total;
    }

    [[nodiscard]] auto yield() -> schedule_operation { return schedule(); }
    auto shutdown() noexcept -> void;
    [[nodiscard]] auto is_shutdown() const -> bool { return m_shutdown_requested.load(std::memory_order::acquire); }
    auto size() const noexcept -> std::size_t { return m_size.load(std::memory_order::acquire); }
    auto empty() const noexcept -> bool { return size() == 0; }

    auto queue_size() const noexcept -> std::size_t {
        std::atomic_thread_fence(std::memory_order::acquire);
        return m_queue.size();
    }
    auto queue_empty() const noexcept -> bool { return queue_size() == 0; }

  private:
    options m_opts;
    std::vector<std::thread> m_threads;
    std::mutex m_wait_mutex;
    std::condition_variable_any m_wait_cv;
    std::deque<std::coroutine_handle<>> m_queue;
    auto executor(std::size_t idx) -> void;
    auto schedule_impl(std::coroutine_handle<> handle) noexcept -> void;
    std::atomic<std::size_t> m_size{0};
    std::atomic<bool> m_shutdown_requested{false};
};

} // namespace silicon::thread
