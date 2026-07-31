module;

#include <chrono>
#include <ctime>
#include <vector>

#if defined(__linux__)
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

export module silicon.coroutine:detail.io_notifier_epoll;

import :detail.poll_info;
import :fd;
import :poll;

// 始终导出一个与平台无关的合法符号，确保该模块分区在非目标平台上也非空
// （clang 22 对零导出的模块分区接口单元处理有 bug，会生成损坏 BMI）。
export namespace silicon::coroutine::detail {
inline constexpr bool io_notifier_epoll_available = true;
}

#if defined(__linux__)
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
