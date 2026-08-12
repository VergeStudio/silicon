module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <stdexcept>
#include <chrono>
#include <utility>
#include <expected>



#include "silicon/coroutine/common.h"

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

export module silicon.coroutine:task_container;

import silicon.scheduler;
import silicon.scheduler.task;
import :mutex;
export namespace silicon::coroutine {

template<concepts::executor executor_type>
class task_container
{
public:
    /**
     * @param e Tasks started in the container are scheduled onto this executor.  For tasks created
     *           from a scheduler, this would usually be that scheduler instance.
     */
  private:
    explicit task_container(std::shared_ptr<executor_type> e) : m_p(std::make_unique<Impl>()) {
        m_p->m_executor = std::move(e);
    }

  public:
    /**
     * @brief 构造可失败工厂：校验 executor 非空，成功后返回
     *        std::unique_ptr<task_container>。失败时返回
     *        std::unexpected(coroutine_error::kNullExecutor)。
     */
    static auto create(std::shared_ptr<executor_type> e)
            -> std::expected<std::unique_ptr<task_container<executor_type>>, std::error_code> {
        if (e == nullptr) {
            return std::unexpected(make_error_code(coroutine_error::kNullExecutor));
        }
        // create() 为成员函数，可访问私有构造；std::make_unique 无 friend 权限故用 new。
        return std::unique_ptr<task_container<executor_type>>(
            new task_container<executor_type>(std::move(e)));
    }

    task_container(const task_container&)                    = delete;
    task_container(task_container&&)                         = delete;
    auto operator=(const task_container&) -> task_container& = delete;
    auto operator=(task_container&&) -> task_container&      = delete;
    ~task_container()
    {
        // This will hang the current thread.. but if tasks are not complete thats also pretty bad.
        while (!empty())
        {
            // Sleep a bit so the cpu doesn't totally churn.
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }

    /**
     * Stores a user task and starts its execution on the container's thread pool.
     * @param user_task The scheduled user's task to store in this task container and start its execution.
     * @return True if the task was succesfully started into the task container. This can fail if the task
     *         is already completed or does not contain a valid coroutine anymore.
     */
    auto start(silicon::scheduler::task<void>&& user_task) -> bool
    {
        m_p->m_size.fetch_add(1, std::memory_order::relaxed);

        auto task = silicon::scheduler::make_task_self_deleting(std::move(user_task));
        // Hook the promise to decrement the size upon its self deletion of the coroutine frame.
        task.promise().user_final_suspend([this]() -> void { m_p->m_size.fetch_sub(1, std::memory_order::release); });
        return m_p->m_executor->resume(task.handle());
    }

    /**
     * @return The number of active tasks in the container.
     */
    auto size() const -> std::size_t { return m_p->m_size.load(std::memory_order::acquire); }

    /**
     * @return True if there are no active tasks in the container.
     */
    auto empty() const -> bool { return size() == 0; }

    /**
     * Will continue to garbage collect and yield until all tasks are complete.  This method can be
     * co_await'ed to make it easier to wait for the task container to have all its tasks complete.
     *
     * This does not shut down the task container, but can be used when shutting down, or if your
     * logic requires all the tasks contained within to complete, it is similar to latch.
     */
    auto yield_until_empty() -> silicon::scheduler::task<void>
    {
        while (!empty())
        {
            co_await m_p->m_executor->yield();
        }
    }

private:
    struct Impl {
      public:
        /// The number of alive tasks.
        std::atomic<std::size_t> m_size{};
        /// The executor to schedule tasks that have just started.
        std::shared_ptr<executor_type> m_executor{nullptr};
    };

    std::unique_ptr<Impl> m_p;
};

} // namespace silicon::coroutine
