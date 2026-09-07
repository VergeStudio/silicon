module;

#include <coroutine>
#include <cstddef>
#include <memory>

#include <silicon/common.h>
export module silicon.scheduler:parallel_scheduler;

import silicon.scheduler.task;
import :facade;
import :thread_pool;

export namespace silicon::scheduler {

class SILICON_CORE_API parallel_scheduler final {
  private:
    struct impl;
    std::unique_ptr<impl> m_impl;

  public:
    parallel_scheduler();
    ~parallel_scheduler();

    parallel_scheduler(const parallel_scheduler &) = delete;
    parallel_scheduler(parallel_scheduler &&) = delete;
    parallel_scheduler & operator=(const parallel_scheduler &) = delete;
    parallel_scheduler & operator=(parallel_scheduler &&) = delete;

    [[nodiscard]] std::size_t thread_count() const noexcept ;

    bool spawn_detached(task<void> &&task) noexcept ;
    task<void> spawn_joinable(task<void> &&t) noexcept ;
    bool resume(std::coroutine_handle<>) noexcept ;
    void shutdown() noexcept ;
    bool is_shutdown() const ;
    std::size_t size() const noexcept ;
    bool empty() const noexcept { return size() == 0; }

    static parallel_scheduler & get_parallel_scheduler() ;
};

}
