module;

#include <atomic>
#include <memory>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll_info_impl;

#if defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)

namespace silicon::scheduler {

class timer_handle::impl {
  public:
    int m_fd{-1};
    const void *m_timer_handle_ptr{nullptr};
};

static auto kqueue_current_timer_fd = std::atomic<int>{0};

timer_handle::timer_handle(const void *timer_handle_ptr, io_notifier &notifier)
    : m_p(std::make_unique<impl>()) {
    m_p->m_fd = kqueue_current_timer_fd++;
    m_p->m_timer_handle_ptr = timer_handle_ptr;
    (void)notifier;
}

auto timer_handle::get_fd() const -> int {
    return m_p->m_fd;
}

const void * timer_handle::get_inner() const {
    return m_p->m_timer_handle_ptr;
}

timer_handle::~timer_handle() = default;

}

#endif
