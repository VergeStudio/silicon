module;

#include <stdexcept>
#include <chrono>
#include <utility>
#include <expected>
#include <system_error>

#include "silicon/common.h"

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

export module silicon.coroutine:task_container;

import silicon.scheduler;
import silicon.scheduler.task;
import silicon.coroutine.error;
import silicon.time;
import :mutex;
export namespace silicon::coroutine {

template<silicon::scheduler::concepts::executor executor_type>
class task_container
{
public:

  private:
    explicit task_container(std::shared_ptr<executor_type> e) : m_p(std::make_unique<impl>()) {
        m_p->m_executor = std::move(e);
    }

  public:

    static std::expected<std::unique_ptr<task_container<executor_type>>, std::error_code> create(std::shared_ptr<executor_type> e) {
        if (e == nullptr) {
            return std::unexpected(make_error_code(coroutine_error::kNullExecutor));
        }

        return std::unique_ptr<task_container<executor_type>>(
            new task_container<executor_type>(std::move(e)));
    }

    task_container(const task_container&)                    = delete;
    task_container(task_container&&)                         = delete;
    task_container& operator=(const task_container&) = delete;
    task_container& operator=(task_container&&) = delete;
    ~task_container()
    {

        while (!empty())
        {

            silicon::time::sleep_for(std::chrono::milliseconds{10});
        }
    }

    bool start(silicon::scheduler::task<void>&& user_task) {
        m_p->m_size.fetch_add(1, std::memory_order::relaxed);

        auto task = silicon::scheduler::make_task_self_deleting(std::move(user_task));

        task.promise().user_final_suspend([this]() -> void { m_p->m_size.fetch_sub(1, std::memory_order::release); });
        return m_p->m_executor->resume(task.handle());
    }

    std::size_t size() const { return m_p->m_size.load(std::memory_order::acquire); }

    bool empty() const { return size() == 0; }

    silicon::scheduler::task<void> yield_until_empty() {
        while (!empty())
        {
            co_await m_p->m_executor->yield();
        }
    }

private:
    struct impl {
      public:

        std::atomic<std::size_t> m_size{};

        std::shared_ptr<executor_type> m_executor{nullptr};
    };

    std::unique_ptr<impl> m_p;
};

}
