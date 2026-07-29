#pragma once

#include "silicon/coroutine/fd.hpp"
#include "silicon/coroutine/io_notifier.hpp"

namespace silicon::coroutine {

namespace detail {

class timer_handle {
    silicon::coroutine::fd_t m_fd;
    const void *m_timer_handle_ptr = nullptr;

  public:
    timer_handle(const void *timer_handle_ptr, io_notifier &notifier);

    silicon::coroutine::fd_t get_fd() const { return m_fd; }

    const void *get_inner() const { return m_timer_handle_ptr; }
};

} // namespace detail

} // namespace silicon::coroutine
