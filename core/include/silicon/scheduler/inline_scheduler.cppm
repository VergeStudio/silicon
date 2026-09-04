module;

#include <coroutine>
#include <cstddef>
#include <memory>

#include <silicon/common.h>
export module silicon.scheduler:inline_scheduler;

import silicon.scheduler.task;
import :facade;

export namespace silicon::scheduler {



















class CORE_API inline_scheduler final {
    struct impl;
    std::unique_ptr<impl> m_impl;

  public:
    inline_scheduler();
    ~inline_scheduler();

    inline_scheduler(const inline_scheduler &) = delete;
    inline_scheduler(inline_scheduler &&) = delete;
    inline_scheduler & operator=(const inline_scheduler &) = delete;
    inline_scheduler & operator=(inline_scheduler &&) = delete;


    bool spawn_detached(task<void> &&task) noexcept ;
    task<void> spawn_joinable(task<void> &&t) noexcept ;
    bool resume(std::coroutine_handle<>) noexcept ;
    void shutdown() noexcept ;
    bool is_shutdown() const ;
    std::size_t size() const noexcept ;
    bool empty() const noexcept { return size() == 0; }
};

}
