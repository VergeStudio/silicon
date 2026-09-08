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

import :poll_info_impl;

#if defined(SILICON_PLATFORM_APPLE) || defined(SILICON_PLATFORM_BSD)
using namespace std::chrono_literals;

namespace silicon::scheduler {

using event_t = struct ::kevent;

struct io_notifier::impl {
    int m_fd{-1};
    bool m_valid{false};
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

        return poll_status::read;
    }

    return poll_status::error;
}

io_notifier::io_notifier(): m_p(std::make_unique<impl>()) {
    m_p->m_fd = ::kqueue();
    m_p->m_valid = (m_p->m_fd != -1);
}

bool io_notifier::is_valid() const noexcept {
    return m_p->m_valid;
}

io_notifier::~io_notifier() = default;

bool io_notifier::watch_timer(const timer_handle &timer, std::chrono::nanoseconds duration) {

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

bool io_notifier::watch(int fd, poll_op op, void *data, bool keep, bool is_cancel_event) {
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

bool io_notifier::unwatch(int fd, poll_op op) {

    if(op == silicon::scheduler::poll_op::read_write) {
        auto event_data = event_t{};

        EV_SET(&event_data, fd, static_cast<int16_t>(silicon::scheduler::poll_op::read), EV_DELETE, 0, 0, nullptr);
        ::kevent(m_p->m_fd, &event_data, 1, nullptr, 0, nullptr);

        EV_SET(&event_data, fd, static_cast<int16_t>(silicon::scheduler::poll_op::write), EV_DELETE, 0, 0, nullptr);
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

bool io_notifier::post(void *) {

    return false;
}

auto io_notifier::native_handle() const -> io_notifier_native_t {
    return m_p->m_fd;
}

}
#endif
