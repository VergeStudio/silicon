module;

#include <memory>

#include <silicon/common.h>
export module silicon.scheduler:timer_handle;

import :fd;
import :poll;
import :time;

import :io_notifier;

export namespace silicon::scheduler {

class SILICON_CORE_API timer_handle {
  private:
    struct impl;
    std::unique_ptr<impl> m_p;

  public:
    timer_handle(const void *timer_handle_ptr, io_notifier &notifier);

    ~timer_handle();

    silicon::scheduler::fd_t get_fd() const;

    const void *get_inner() const;
};

}
