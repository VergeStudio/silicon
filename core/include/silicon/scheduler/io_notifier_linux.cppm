module;

#include <cstdint>

#include <silicon/common.h>
export module silicon.scheduler:io_notifier_linux;

// io_notifier 的平台差异点（Linux/epoll）：native_handle 类型与事件容量。
// 仅在 Linux 平台有内容，其余平台为空分区。

#if defined(SILICON_PLATFORM_LINUX)

export namespace silicon::scheduler {

using io_notifier_native_t = int;

inline constexpr std::size_t io_notifier_max_events = 16;

}

#endif
