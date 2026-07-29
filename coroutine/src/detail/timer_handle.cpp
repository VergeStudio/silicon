module;

module silicon.coroutine;

namespace silicon::coroutine::detail {

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__)

static auto kqueue_current_timer_fd = std::atomic<fd_t>{0};

timer_handle::timer_handle(const void *timer_handle_ptr, io_notifier &notifier)
    : m_fd{kqueue_current_timer_fd++},
      m_timer_handle_ptr(timer_handle_ptr) {
    (void)notifier;
}

#elif defined(__linux__)

timer_handle::timer_handle(const void *timer_handle_ptr, io_notifier &notifier)
    : m_fd(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)),
      m_timer_handle_ptr(timer_handle_ptr) {
    notifier.watch(m_fd, poll_op::read, const_cast<void *>(m_timer_handle_ptr), true);
}

#elif defined(_WIN32)

timer_handle::timer_handle(const void *timer_handle_ptr, io_notifier &notifier)
    : m_fd(-1),
      m_timer_handle_ptr(timer_handle_ptr) {
    // On Windows, timers are managed by the IOCP-based io_notifier.
    // The timer_handle is just a token that carries the poll_info pointer.
    (void)notifier;
}

#endif

} // namespace silicon::coroutine::detail
