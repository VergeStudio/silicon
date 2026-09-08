module;

#include <cstddef>

#include <silicon/common.h>
export module silicon.scheduler:io_notifier_win;

// io_notifier 的平台差异点（Windows/IOCP）：native_handle 类型与事件容量。
// 仅在 Windows 平台有内容，其余平台为空分区。
// native_handle 返回 void*（即 Win32 HANDLE），接口不再向消费方泄漏 windows.h。

#if defined(SILICON_PLATFORM_WINDOWS)

export namespace silicon::scheduler {

using io_notifier_native_t = void *;

inline constexpr std::size_t io_notifier_max_events = 64;

}

#endif
