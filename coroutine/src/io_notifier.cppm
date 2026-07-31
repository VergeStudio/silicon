module;

#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(__linux__)
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

export module silicon.coroutine:io_notifier;

import :detail.poll_info;
import :fd;
import :poll;

// 始终导出的平台无关符号，确保该模块分区在所有平台上都非空
// （clang 22 对零导出的模块分区接口单元处理有 bug，会生成损坏 BMI，
// 进而让消费方随机 clang frontend 崩溃）。
export namespace silicon::coroutine::detail {
inline constexpr bool io_notifier_iocp_available = true;
inline constexpr bool io_notifier_epoll_available = true;
inline constexpr bool io_notifier_kqueue_available = true;
}

// io_notifier 三个平台后端（iocp / epoll / kqueue）的类声明合并到本分区：
// 仅当前平台展开对应类声明，非目标平台编译为空分区（但仍通过上面的 constexpr
// 标志保持非空）。各后端的成员函数定义位于同名 .cpp 实现单元
// （io_notifier_iocp.cpp / io_notifier_epoll.cpp / io_notifier_kqueue.cpp），
// 由宏开关决定是否参与编译。
#if defined(_WIN32)
export namespace silicon::coroutine::detail {

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
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
export namespace silicon::coroutine::detail {

using event_t = struct ::kevent;

class timer_handle;

class io_notifier_kqueue {
    static const constexpr std::size_t m_max_events = 16;

    fd_t m_fd;

    friend class detail::timer_handle;

    static auto event_to_poll_status(const event_t &event) -> poll_status;

  public:
    io_notifier_kqueue();

    io_notifier_kqueue(const io_notifier_kqueue &) = delete;
    io_notifier_kqueue(io_notifier_kqueue &&) = delete;
    auto operator=(const io_notifier_kqueue &) -> io_notifier_kqueue & = delete;
    auto operator=(io_notifier_kqueue &&) -> io_notifier_kqueue & = delete;

    ~io_notifier_kqueue();

    auto watch_timer(const timer_handle &timer, std::chrono::nanoseconds duration) -> bool;

    auto watch(fd_t fd, poll_op op, void *data, bool keep = false) -> bool;

    auto watch(poll_info &pi) -> bool;

    auto unwatch(fd_t fd, poll_op op) -> bool;

    auto unwatch(poll_info &pi) -> bool;

    auto unwatch_timer(const timer_handle &timer) -> bool;

    auto next_events(std::vector<std::pair<poll_info *, poll_status>> &ready_events, std::chrono::milliseconds timeout)
            -> void;

    auto native_handle() const -> fd_t { return m_fd; }
};

} // namespace silicon::coroutine::detail
#elif defined(__linux__)
export namespace silicon::coroutine::detail {

using event_t = struct ::epoll_event;

class timer_handle;

class io_notifier_epoll {
    static const constexpr std::size_t m_max_events = 16;

    fd_t m_fd;

    friend class detail::timer_handle;

    static auto event_to_poll_status(const event_t &event) -> poll_status;

  public:
    io_notifier_epoll();

    io_notifier_epoll(const io_notifier_epoll &) = delete;
    io_notifier_epoll(io_notifier_epoll &&) = delete;
    auto operator=(const io_notifier_epoll &) -> io_notifier_epoll & = delete;
    auto operator=(io_notifier_epoll &&) -> io_notifier_epoll & = delete;

    ~io_notifier_epoll();

    auto watch_timer(const timer_handle &timer, std::chrono::nanoseconds duration) -> bool;

    auto watch(fd_t fd, poll_op op, void *data, bool keep = false, bool is_cancel_event = false) -> bool;

    auto watch(poll_info &pi) -> bool;

    auto unwatch(fd_t fd, poll_op op) -> bool;

    auto unwatch(detail::poll_info &pi) -> bool;

    auto unwatch_timer(const timer_handle &timer) -> bool;

    auto next_events(std::vector<std::pair<poll_info *, poll_status>> &ready_events, std::chrono::milliseconds timeout)
            -> void;
};

} // namespace silicon::coroutine::detail
#endif

export namespace silicon::coroutine {

// 平台专属 io_notifier 后端的公开别名：当前平台暴露对应的 detail 后端类。
#if defined(_WIN32)
using io_notifier = detail::io_notifier_iocp;
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
using io_notifier = detail::io_notifier_kqueue;
#elif defined(__linux__)
using io_notifier = detail::io_notifier_epoll;
#endif

} // namespace silicon::coroutine
