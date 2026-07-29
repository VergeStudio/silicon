#pragma once

#ifndef _WIN32
#    error "io_notifier_iocp.hpp is Windows-only"
#endif

#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "silicon/coroutine/detail/poll_info.hpp"
#include "silicon/coroutine/fd.hpp"
#include "silicon/coroutine/poll.hpp"

namespace silicon::coroutine::detail {

class timer_handle;

class io_notifier_iocp {
    /// Maximum events to batch in a single next_events call.
    static constexpr std::size_t m_max_events = 64;

    /// The IOCP handle.
    HANDLE m_iocp;

    /// Mutex protecting the watched-fds tracking structures.
    std::mutex m_mutex;

    /// Currently watched socket file descriptors and their watch mode.
    struct watch_entry {
        poll_op op;
        void *data;
        bool keep;
        bool is_cancel_event;
    };
    std::unordered_map<fd_t, watch_entry> m_watched_fds;

    friend class detail::timer_handle;

    static auto event_to_poll_status(WSANETWORKEVENTS &net_events, poll_op requested_op) -> poll_status;

    /// Clean up any internal resources for a given fd.
    auto remove_fd(fd_t fd) -> void;

  public:
    io_notifier_iocp();

    io_notifier_iocp(const io_notifier_iocp &) = delete;
    io_notifier_iocp(io_notifier_iocp &&) = delete;
    auto operator=(const io_notifier_iocp &) -> io_notifier_iocp & = delete;
    auto operator=(io_notifier_iocp &&) -> io_notifier_iocp & = delete;

    ~io_notifier_iocp();

    auto watch_timer(const timer_handle &timer, std::chrono::nanoseconds duration) -> bool;

    auto watch(fd_t fd, poll_op op, void *data, bool keep = false, bool is_cancel_event = false) -> bool;

    auto watch(poll_info &pi) -> bool;

    auto unwatch(fd_t fd, poll_op op) -> bool;

    auto unwatch(poll_info &pi) -> bool;

    auto unwatch_timer(const timer_handle &timer) -> bool;

    auto next_events(std::vector<std::pair<poll_info *, poll_status>> &ready_events, std::chrono::milliseconds timeout)
            -> void;

    auto native_handle() const -> HANDLE { return m_iocp; }
};

} // namespace silicon::coroutine::detail
