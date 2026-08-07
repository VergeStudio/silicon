module;

#include <chrono>
#include <cstdint>
#include <memory>
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

export module silicon.scheduler:io_notifier;

import silicon.coroutine;

import :detail.poll_info;

// timer_handle 仅以引用形式出现在 watch_timer / unwatch_timer 的签名中（不完整类型即可），
// 故只需前向声明，无需 import 整个 :detail.timer_handle 分区（避免分区间循环依赖）。
namespace silicon::scheduler::detail {
export class timer_handle;
}

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

export namespace silicon::scheduler {

/// 平台无关的 I/O 就绪通知器。
///
/// 每个平台只暴露**同一个** `io_notifier` 类，对外提供完全一致的接口；
/// 平台专属后端（Windows=IOCP / BSD·macOS=kqueue / Linux=epoll）隐藏在私有的嵌套
/// `struct Impl`（PIMPL）之中。`Impl` 的实体定义位于同名 `.cpp` 实现单元
/// （io_notifier_iocp.cpp / io_notifier_kqueue.cpp / io_notifier_epoll.cpp），因此
/// HANDLE / WSANETWORKEVENTS / kevent / epoll_event 等平台专属状态**绝不出现在本接口单元**。
/// 任何触及 Impl 成员的方法都必须在各自平台的 `.cpp` 中**非内联**实现。
class io_notifier {
    struct Impl;
    std::unique_ptr<Impl> m_p;

    friend class detail::timer_handle;

    // 单次 epoll_wait / kevent 的最大事件数；Windows 的 IOCP 完成包数量级不同，取较大值。
    static constexpr std::size_t m_max_events =
#if defined(_WIN32)
        64;
#else
        16;
#endif

    auto remove_fd(fd_t fd) -> void;

  public:
    io_notifier();

    io_notifier(const io_notifier &) = delete;
    io_notifier(io_notifier &&) = delete;
    auto operator=(const io_notifier &) -> io_notifier & = delete;
    auto operator=(io_notifier &&) -> io_notifier & = delete;

    ~io_notifier();

    auto watch_timer(const detail::timer_handle &timer, std::chrono::nanoseconds duration) -> bool;

    auto watch(fd_t fd, poll_op op, void *data, bool keep = false, bool is_cancel_event = false) -> bool;

    auto watch(detail::poll_info &pi) -> bool;

    auto unwatch(fd_t fd, poll_op op) -> bool;

    auto unwatch(detail::poll_info &pi) -> bool;

    auto unwatch_timer(const detail::timer_handle &timer) -> bool;

    auto next_events(std::vector<std::pair<detail::poll_info *, poll_status>> &ready_events,
                     std::chrono::milliseconds timeout) -> void;

    auto native_handle() const ->
#if defined(_WIN32)
        HANDLE;
#else
        fd_t;
#endif
};

} // namespace silicon::scheduler
