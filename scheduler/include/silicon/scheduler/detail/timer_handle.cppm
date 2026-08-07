module;

#include <memory>


export module silicon.scheduler:detail.timer_handle;

import silicon.coroutine;

import :io_notifier;

// 复用 silicon.coroutine 的基础 I/O 类型（不 export，仅本单元内简化书写）。
namespace silicon::scheduler {
using silicon::coroutine::fd_t;
using silicon::coroutine::poll_op;
using silicon::coroutine::poll_op_readable;
using silicon::coroutine::poll_op_writeable;
using silicon::coroutine::poll_status;
using silicon::coroutine::poll_stop_token;
using silicon::coroutine::time_point;
} // namespace silicon::scheduler

export namespace silicon::scheduler {

namespace detail {

class timer_handle {
    struct Impl;
    std::unique_ptr<Impl> m_p;

  public:
    timer_handle(const void *timer_handle_ptr, io_notifier &notifier);

    ~timer_handle();

    silicon::coroutine::fd_t get_fd() const;

    const void *get_inner() const;
};

} // namespace detail

} // namespace silicon::scheduler
