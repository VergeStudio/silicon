#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "silicon/scheduler/task/detail/task_self_deleting.hpp"
#include "silicon/scheduler/task/event.hpp"
#include "silicon/scheduler/task/task.hpp"

namespace silicon::scheduler::task {

template<typename executor_type>
class task_group {
  public:
    explicit task_group(executor_type *executor)
        : m_executor(executor) {
        if(executor == nullptr) {
            throw std::runtime_error{"task_group cannot have a nullptr executor"};
        }
    }

    explicit task_group(executor_type *executor, silicon::scheduler::task::task<void> &&task)
        : task_group(executor) {
        (void)start(std::forward<silicon::scheduler::task::task<void>>(task));
    }

    template<typename range_type>
    explicit task_group(executor_type *executor, range_type tasks): task_group(executor) {
        for(auto &t: tasks) {
            (void)start(std::move(t));
        }
    }

    explicit task_group(std::unique_ptr<executor_type> &executor)
        : m_executor(executor.get()) {}

    explicit task_group(std::unique_ptr<executor_type> &executor, silicon::scheduler::task::task<void> &&task)
        : task_group(executor.get(), std::forward<silicon::scheduler::task::task<void>>(task)) {}

    template<typename range_type>
    explicit task_group(std::unique_ptr<executor_type> &executor, range_type tasks)
        : task_group(executor.get(), std::forward<range_type>(tasks)) {}

    task_group(const task_group &) = delete;
    task_group(task_group &&) = delete;
    auto operator=(const task_group &) -> task_group & = delete;
    auto operator=(task_group &&) -> task_group & = delete;

    ~task_group() {
        while(!empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }

    [[nodiscard]] auto start(silicon::scheduler::task::task<void> &&task) -> bool {
        m_on_empty_event.reset();
        m_size.fetch_add(1, std::memory_order::release);
        auto wrapper_task = detail::make_task_self_deleting(std::move(task));
        wrapper_task.promise().user_final_suspend([this]() -> void { count_down(); });

        if(!m_executor->resume(wrapper_task.handle())) {
            count_down();
            return false;
        }
        return true;
    }

    [[nodiscard]] auto size() const -> std::size_t { return m_size.load(std::memory_order::acquire); }
    [[nodiscard]] auto empty() const -> bool { return size() == 0; }

    auto operator co_await() const noexcept -> task_event::awaiter { return m_on_empty_event.operator co_await(); }

  private:
    executor_type *m_executor{nullptr};
    std::atomic<uint64_t> m_size{};
    silicon::scheduler::task::task_event m_on_empty_event{true};

    auto count_down() -> void {
        if(m_size.fetch_sub(1, std::memory_order::acq_rel) == 1) {
            m_on_empty_event.set();
        }
    }
};

} // namespace silicon::scheduler::task
