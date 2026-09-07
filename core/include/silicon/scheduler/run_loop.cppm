module;

#include <coroutine>
#include <cstddef>
#include <memory>

#include <silicon/common.h>
export module silicon.scheduler:run_loop;

import silicon.scheduler.task;
import :facade;

export namespace silicon::scheduler {

class SILICON_CORE_API run_loop final {
  private:
    struct impl;
    std::unique_ptr<impl> m_impl;

  public:
    run_loop();
    ~run_loop();

    run_loop(const run_loop &) = delete;
    run_loop(run_loop &&) = delete;
    run_loop & operator=(const run_loop &) = delete;
    run_loop & operator=(run_loop &&) = delete;

    void run() noexcept ;

    void finish() noexcept ;

    bool spawn_detached(task<void> &&task) noexcept ;
    task<void> spawn_joinable(task<void> &&t) noexcept ;
    bool resume(std::coroutine_handle<>) noexcept ;
    void shutdown() noexcept ;
    bool is_shutdown() const ;
    std::size_t size() const noexcept ;
    bool empty() const noexcept { return size() == 0; }
};

}
