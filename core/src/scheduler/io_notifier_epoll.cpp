module;

#if defined(SILICON_PLATFORM_LINUX)
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

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

#if defined(SILICON_PLATFORM_LINUX)
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

using event_t = struct ::epoll_event;

// ---------------------------------------------------------------------------
// PIMPL: epoll backend state for io_notifier.
// ---------------------------------------------------------------------------
struct io_notifier::impl {
    fd_t m_fd{-1};
    bool m_valid{false};
};

/**
 * Encode the state needed to rewind the correct poll info after an event occured into the user data of an epoll event.
 *
 * We need a pointer to the poll_info awaitable to resume the awaiting coroutine. Additionally we need to know if the
 * captured event came from the file descriptor of the poll_info, in which case we want to decode the poll_status from
 * the event, or from a registered cancellation token, in which case we want to return a cancelled poll status.
 */
static uint64_t encode_udata(bool keep_registered, bool is_cancel_event, void *udata) {
    // Pointers on 64 bit unix machines take up to 48 bit right now. So we have some bits left to encode the boolean to
    // indicate if this is a cancellation event descriptor at the highest bit.
    return (((uint64_t)keep_registered) << 63) | (((uint64_t)is_cancel_event) << 62) |
           (reinterpret_cast<uintptr_t>(udata) & 0x3FFFFFFFFFFFFFFFULL);
}

/**
 * Decode the state that was encoded into the epoll events user data field.
 *
 * For details see documentation of `silicon::scheduler::encode_udata(bool, bool, void*)` above.
 */
static std::tuple<bool, bool, void *> decode_udata(uint64_t encoded) {
    bool keep_registered = (bool)(encoded >> 63);
    bool is_cancel_event = (bool)((encoded >> 62) & 0x1);
    void *udata = reinterpret_cast<void *>(encoded & 0xFFFFFFFFFFFFULL);
    return std::make_tuple(keep_registered, is_cancel_event, udata);
}

static poll_status event_to_poll_status(const event_t &event) {
    if(event.events & static_cast<uint32_t>(poll_op::read)) {
        return poll_status::read;
    }
    if(event.events & static_cast<uint32_t>(poll_op::write)) {
        return poll_status::write;
    } else if(event.events & EPOLLERR) {
        return poll_status::error;
    } else if(event.events & EPOLLRDHUP || event.events & EPOLLHUP) {
        return poll_status::closed;
    }
    return poll_status::error;
}

io_notifier::io_notifier(): m_p(std::make_unique<impl>()) {
    m_p->m_fd = ::epoll_create1(EPOLL_CLOEXEC);
    m_p->m_valid = (m_p->m_fd != -1);
}

bool io_notifier::is_valid() const noexcept {
    return m_p->m_valid;
}

io_notifier::~io_notifier() = default;

bool io_notifier::watch_timer(const timer_handle &timer, std::chrono::nanoseconds duration) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    duration -= seconds;
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

    // As a safeguard if both values end up as zero (or negative) then trigger the timeout
    // immediately as zero disarms timerfd according to the man pages and negative values
    // will result in an error return value.
    if(seconds <= 0s) {
        seconds = 0s;
        if(nanoseconds <= 0ns) {
            nanoseconds = 1ns;
        }
    }

    itimerspec ts{};
    ts.it_value.tv_sec = seconds.count();
    ts.it_value.tv_nsec = nanoseconds.count();
    return ::timerfd_settime(timer.get_fd(), 0, &ts, nullptr) != -1;
}

bool io_notifier::watch(fd_t fd, poll_op op, void *data, bool keep, bool is_cancel_event) {
    auto event_data = event_t{};
    event_data.events = static_cast<uint32_t>(op) | EPOLLRDHUP;
    event_data.data.u64 = encode_udata(keep, is_cancel_event, data);

    if(!keep) {
        event_data.events |= EPOLLONESHOT;
    } else {
        // For events being kept in a scheduler they need to be edge triggered or they'll constantly wake-up the event
        // loop.
        event_data.events |= EPOLLET;
    }

    return ::epoll_ctl(m_p->m_fd, EPOLL_CTL_ADD, fd, &event_data) != -1;
}

bool io_notifier::watch(poll_info &pi) {
    watch(pi.m_p->m_fd, pi.m_p->m_op, static_cast<void *>(&pi), false, false);

    if(pi.m_p->m_cancel_trigger.has_value()) {
        watch(pi.m_p->m_cancel_trigger.value().native_handle(), poll_op::read, static_cast<void *>(&pi), false, true);
    }

    return true;
}

bool io_notifier::unwatch(fd_t fd, poll_op) {
    return ::epoll_ctl(m_p->m_fd, EPOLL_CTL_DEL, fd, nullptr) != -1;
}

bool io_notifier::unwatch(poll_info &pi) {
    return unwatch(pi.m_p->m_fd, pi.m_p->m_op);
}

bool io_notifier::unwatch_timer(const timer_handle &timer) {
    // Setting these values to zero disables the timer.
    itimerspec ts{};
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 0;
    return ::timerfd_settime(timer.get_fd(), 0, &ts, nullptr) != -1;
}

void io_notifier::next_events(
        std::vector<std::pair<poll_info *, poll_status>> &ready_events, std::chrono::milliseconds timeout
) {
    auto ready_set = std::array<event_t, m_max_events>{};
    int num_ready = ::epoll_wait(m_p->m_fd, ready_set.data(), ready_set.size(), timeout.count());
    for(int i = 0; i < num_ready; ++i) {
        auto [keep_registered, is_cancel_event, udata] = decode_udata(ready_set[i].data.u64);
        auto *pi = static_cast<poll_info *>(udata);

        // If the event issuing fd is the same as the fd of the cancellation trigger of the registered poll_info we
        // this operation was cancelled by the user.
        if(is_cancel_event) {
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
    // epoll 后端无完成包通道：completion worker 经内部 completion pipe + 哨兵
    // udata 唤醒驱动（io_scheduler_completion.cpp 的 wake_driver），此处无需
    // 也无法投递。返回 false 仅表示"本后端不支持 post"。
    return false;
}

auto io_notifier::native_handle() const -> fd_t {
    return m_p->m_fd;
}

} // namespace silicon::scheduler
#endif
