module;

#include <memory>


export module silicon.scheduler:timer_handle;

import :fd;
import :poll;
import :time;

import :io_notifier;

// 基础 I/O 类型已随调度原语迁入本模块（命名空间仍为 silicon::coroutine），
// 此处仅在本单元内引入简化书写，不 export。
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



class timer_handle {
    struct impl;
    std::unique_ptr<impl> m_p;

  public:
    timer_handle(const void *timer_handle_ptr, io_notifier &notifier);

    ~timer_handle();

    silicon::coroutine::fd_t get_fd() const;

    const void *get_inner() const;
};



} // namespace silicon::scheduler
