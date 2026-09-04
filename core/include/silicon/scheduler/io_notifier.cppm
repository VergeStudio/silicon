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

#include <silicon/common.h>
export module silicon.scheduler:io_notifier;

import :fd;
import :poll;
import :time;

import :poll_info;

namespace silicon::scheduler {
export class timer_handle;
}

export namespace silicon::scheduler {

class CORE_API io_notifier {
    struct impl;
    std::unique_ptr<impl> m_p;

    friend class timer_handle;

    static constexpr std::size_t m_max_events =
#if defined(SILICON_PLATFORM_WINDOWS)
        64;
#else
        16;
#endif

    void remove_fd(fd_t) ;

  public:
    io_notifier();

    [[nodiscard]] bool is_valid() const noexcept;

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

    bool post(void *) ;

#if defined(SILICON_PLATFORM_WINDOWS)
        HANDLE native_handle() const ;
#else
        fd_t native_handle() const ;
#endif
};

}
