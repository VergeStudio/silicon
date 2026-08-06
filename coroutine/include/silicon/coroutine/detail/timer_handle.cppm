module;

#include <memory>


export module silicon.coroutine:detail.timer_handle;

import :fd;
import :io_notifier;

export namespace silicon::coroutine {

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

} // namespace silicon::coroutine
