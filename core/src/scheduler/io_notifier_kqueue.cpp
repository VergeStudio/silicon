module;

#if defined(SILICON_PLATFORM_APPLE) || defined(SILICON_PLATFORM_BSD)
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <coroutine>
#include <map>
#include <optional>

module silicon.scheduler;
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫。
#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll_info_impl;
#include "poll_info_impl.hpp"

#if defined(SILICON_PLATFORM_APPLE) || defined(SILICON_PLATFORM_BSD)
using namespace std::chrono_literals;

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

using event_t = struct ::kevent;

// ---------------------------------------------------------------------------
// PIMPL: kqueue backend state for io_notifier.
// ---------------------------------------------------------------------------
struct io_notifier::impl {
    fd_t m_fd{-1};
};

static poll_status event_to_poll_status(const event_t &event) {
    if((event.filter == EVFILT_READ || event.filter == EVFILT_WRITE || event.filter == EVFILT_TIMER) &&
       event.flags & EV_EOF) {
        return poll_status::closed;
    } else if(
            (event.filter == EVFILT_READ || event.filter == EVFILT_WRITE || event.filter == EVFILT_TIMER) &&
            event.flags & EV_ERROR
    ) {
        return poll_status::error;
    } else if(event.filter == EVFILT_READ) {
        return poll_status::read;
    } else if(event.filter == EVFILT_WRITE) {
        return poll_status::write;
    } else if(event.filter == EVFILT_TIMER) {
        // Due timer is handled like a read event by the lib
        return poll_status::read;
    }

    throw std::runtime_error{"invalid kqueue state"};
}

io_notifier::io_notifier(): m_p(std::make_unique<impl>()) {
    m_p->m_fd = ::kqueue();
}

io_notifier::~io_notifier() = default;

bool io_notifier::watch_timer(const timer_handle &timer, std::chrono::nanoseconds duration) {
    // Prevent negative durations for the timeout as they will result in an error. 0 will fire in the next instance
    // possible.
    if(duration < 0ns) {
        duration = 0ns;
    }

    auto event_data = event_t{};
    EV_SET(
            &event_data,
            timer.get_fd(),
            EVFILT_TIMER,
            EV_ADD | EV_CLEAR | EV_ONESHOT,
            NOTE_NSECONDS,
            duration.count(),
            const_cast<void *>(timer.get_inner())
    );

    return ::kevent(m_p->m_fd, &event_data, 1, nullptr, 0, nullptr) != -1;
}

bool io_notifier::watch(fd_t fd, poll_op op, void *data, bool keep, bool is_cancel_event) {
    (void)is_cancel_event;
    auto event_data = event_t{};
    auto mode = EV_ADD | EV_CLEAR | EV_ENABLE;
    if(!keep) {
        mode |= EV_ONESHOT;
    }

    EV_SET(&event_data, fd, static_cast<int16_t>(op), mode, 0, 0, data);
    return ::kevent(m_p->m_fd, &event_data, 1, nullptr, 0, nullptr) != -1;
}

bool io_notifier::watch(poll_info &pi) {
    // For read-write event, we need to register both event types separately to the kqueue
    if(pi.m_p->m_op == poll_op::read_write) {
        if(!watch(pi.m_p->m_fd, poll_op::read, static_cast<void *>(&pi), false, false) ||
           !watch(pi.m_p->m_fd, poll_op::write, static_cast<void *>(&pi), false, false)) {
            return false;
        }
    } else {
        if(!watch(pi.m_p->m_fd, pi.m_p->m_op, static_cast<void *>(&pi), false, false)) {
            return false;
        }
    }

    if(pi.m_p->m_cancel_trigger.has_value()) {
        watch(pi.m_p->m_cancel_trigger.value().native_handle(), poll_op::read, static_cast<void *>(&pi), false, false);
    }

    return true;
}

bool io_notifier::unwatch(fd_t fd, poll_op op) {
    // For read-write event, we need to de-register both event types separately to the kqueue
    if(op == silicon::coroutine::poll_op::read_write) {
        auto event_data = event_t{};

        EV_SET(&event_data, fd, static_cast<int16_t>(silicon::coroutine::poll_op::read), EV_DELETE, 0, 0, nullptr);
        ::kevent(m_p->m_fd, &event_data, 1, nullptr, 0, nullptr);

        EV_SET(&event_data, fd, static_cast<int16_t>(silicon::coroutine::poll_op::write), EV_DELETE, 0, 0, nullptr);
        ::kevent(m_p->m_fd, &event_data, 1, nullptr, 0, nullptr);

        return true;
    } else {
        auto event_data = event_t{};
        EV_SET(&event_data, fd, static_cast<int16_t>(op), EV_DELETE, 0, 0, nullptr);
        return ::kevent(m_p->m_fd, &event_data, 1, nullptr, 0, nullptr) != -1;
    }
}

bool io_notifier::unwatch(poll_info &pi) {
    return unwatch(pi.m_p->m_fd, pi.m_p->m_op);
}

bool io_notifier::unwatch_timer(const timer_handle &timer) {
    auto event_data = event_t{};
    EV_SET(&event_data, timer.get_fd(), EVFILT_TIMER, EV_DELETE, 0, 0, nullptr);
    return ::kevent(m_p->m_fd, &event_data, 1, nullptr, 0, nullptr) != -1;
}

void io_notifier::next_events(
        std::vector<std::pair<poll_info *, poll_status>> &ready_events, std::chrono::milliseconds timeout
) {
    auto ready_set = std::array<event_t, m_max_events>{};
    const auto timeout_as_secs = std::chrono::duration_cast<std::chrono::seconds>(timeout);
    auto timeout_spec = ::timespec{
            .tv_sec = timeout_as_secs.count(),
            .tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(timeout - timeout_as_secs).count(),
    };
    const int num_ready = ::kevent(
            m_p->m_fd, nullptr, 0, ready_set.data(), std::min(ready_set.size(), ready_events.capacity()), &timeout_spec
    );
    for(int i = 0; i < num_ready; i++) {
        auto *pi = static_cast<poll_info *>(ready_set[i].udata);

        auto keep_registered = !(ready_set[i].flags & EV_ONESHOT);

        // If the event issuing fd is the same as the fd of the cancellation trigger of the registered poll_info we
        // this operation was cancelled by the user.
        if(pi->m_p->m_cancel_trigger.has_value() &&
           ready_set[i].ident == static_cast<uintptr_t>(pi->m_p->m_cancel_trigger.value().native_handle())) {
            ready_events.emplace_back(pi, poll_status::cancelled);
            if(!keep_registered) {
                unwatch(*pi);
            }
        } else {
            ready_events.emplace_back(pi, event_to_poll_status(ready_set[i]));
            if(pi->m_p->m_cancel_trigger.has_value() && !keep_registered) {
                unwatch(pi->m_p->m_cancel_trigger.value().native_handle(), poll_op::read);
            }
        }
    }
}

auto io_notifier::native_handle() const -> fd_t {
    return m_p->m_fd;
}

} // namespace silicon::scheduler
#endif
