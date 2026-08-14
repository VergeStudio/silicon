module;

#include <memory>

module silicon.scheduler;

import :poll_info_impl;

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

namespace silicon::scheduler {

class timer_handle::Impl {
  public:
    fd_t m_fd{-1};
    const void *m_timer_handle_ptr{nullptr};
};

#if defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)

static auto kqueue_current_timer_fd = std::atomic<fd_t>{0};

timer_handle::timer_handle(const void *timer_handle_ptr, io_notifier &notifier)
    : m_p(std::make_unique<Impl>()) {
    m_p->m_fd = kqueue_current_timer_fd++;
    m_p->m_timer_handle_ptr = timer_handle_ptr;
    (void)notifier;
}

#elif defined(SILICON_PLATFORM_LINUX)

timer_handle::timer_handle(const void *timer_handle_ptr, io_notifier &notifier)
    : m_p(std::make_unique<Impl>()) {
    m_p->m_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    m_p->m_timer_handle_ptr = timer_handle_ptr;
    notifier.watch(m_p->m_fd, poll_op::read, const_cast<void *>(m_p->m_timer_handle_ptr), true);
}

#elif defined(SILICON_PLATFORM_WINDOWS)

timer_handle::timer_handle(const void *timer_handle_ptr, io_notifier &notifier)
    : m_p(std::make_unique<Impl>()) {
    // On Windows, timers are managed by the IOCP-based io_notifier.
    // The timer_handle is just a token that carries the poll_info pointer.
    m_p->m_fd = -1;
    m_p->m_timer_handle_ptr = timer_handle_ptr;
    (void)notifier;
}

#endif

auto timer_handle::get_fd() const -> fd_t {
    return m_p->m_fd;
}

auto timer_handle::get_inner() const -> const void * {
    return m_p->m_timer_handle_ptr;
}

timer_handle::~timer_handle() = default;

} // namespace silicon::scheduler
