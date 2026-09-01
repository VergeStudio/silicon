module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#if defined(SILICON_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#elif defined(SILICON_PLATFORM_APPLE) || defined(SILICON_PLATFORM_BSD)
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(SILICON_PLATFORM_LINUX)
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

export module silicon.scheduler:io_notifier;

import :fd;
import :poll;
import :time;

import :poll_info;

// timer_handle 仅以引用形式出现在 watch_timer / unwatch_timer 的签名中（不完整类型即可），
// 故只需前向声明，无需 import 整个 :timer_handle 分区（避免分区间循环依赖）。
namespace silicon::scheduler {
export class timer_handle;
}

// 基础 I/O 类型已随调度原语迁入本模块（命名空间仍为 silicon::coroutine），
// 此处仅在本单元内引入简化书写，不 export。
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
/// `struct impl`（PIMPL）之中。`impl` 的实体定义位于同名 `.cpp` 实现单元
/// （io_notifier_iocp.cpp / io_notifier_kqueue.cpp / io_notifier_epoll.cpp），因此
/// HANDLE / WSANETWORKEVENTS / kevent / epoll_event 等平台专属状态**绝不出现在本接口单元**。
/// 任何触及 impl 成员的方法都必须在各自平台的 `.cpp` 中**非内联**实现。
class io_notifier {
    struct impl;
    std::unique_ptr<impl> m_p;

    friend class timer_handle;

    // 单次 epoll_wait / kevent 的最大事件数；Windows 的 IOCP 完成包数量级不同，取较大值。
    static constexpr std::size_t m_max_events =
#if defined(SILICON_PLATFORM_WINDOWS)
        64;
#else
        16;
#endif

    void remove_fd(fd_t) ;

  public:
    io_notifier();

    io_notifier(const io_notifier &) = delete;
    io_notifier(io_notifier &&) = delete;
    io_notifier & operator=(const io_notifier &) = delete;
    io_notifier & operator=(io_notifier &&) = delete;

    ~io_notifier();

    bool watch_timer(const timer_handle &, std::chrono::nanoseconds) ;

    bool watch(fd_t, poll_op, void *, bool = false, bool = false) ;

    bool watch(poll_info &) ;

    bool unwatch(fd_t, poll_op) ;

    bool unwatch(poll_info &) ;

    bool unwatch_timer(const timer_handle &) ;

    void next_events(std::vector<std::pair<poll_info *, poll_status>> &,
                     std::chrono::milliseconds) ;

#if defined(SILICON_PLATFORM_WINDOWS)
        HANDLE native_handle() const ;
#else
        fd_t native_handle() const ;
#endif
};

} // namespace silicon::scheduler
