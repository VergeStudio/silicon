module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include <silicon/common.h>
export module silicon.scheduler:io_notifier;

import :poll;
import silicon.time;

import :poll_info;

// 平台差异点（native_handle 返回类型、事件容量）定义在平台分区中，
// 此处按当前平台再导出，消费方无感知。
#if defined(SILICON_PLATFORM_LINUX)
export import :io_notifier_linux;
#elif defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)
export import :io_notifier_kqueue;
#elif defined(SILICON_PLATFORM_WINDOWS)
export import :io_notifier_win;
#endif

namespace silicon::scheduler {
export class timer_handle;
}

export namespace silicon::scheduler {

class SILICON_CORE_API io_notifier {
  private:
    struct impl;
    std::unique_ptr<impl> m_p;

    friend class timer_handle;

    static constexpr std::size_t m_max_events = io_notifier_max_events;

    void remove_fd(int) ;

  public:
    io_notifier();

    [[nodiscard]] bool is_valid() const noexcept;

    io_notifier(const io_notifier &) = delete;
    io_notifier(io_notifier &&) = delete;
    io_notifier & operator=(const io_notifier &) = delete;
    io_notifier & operator=(io_notifier &&) = delete;

    ~io_notifier();

    bool watch_timer(const timer_handle &, std::chrono::nanoseconds) ;

    bool watch(int, poll_op, void *, bool = false, bool = false) ;

    bool watch(poll_info &) ;

    bool unwatch(int, poll_op) ;

    bool unwatch(poll_info &) ;

    bool unwatch_timer(const timer_handle &) ;

    void next_events(std::vector<std::pair<poll_info *, poll_status>> &,
                     std::chrono::milliseconds) ;

    bool post(void *) ;

    io_notifier_native_t native_handle() const ;
};

}
