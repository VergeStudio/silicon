module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <winsock2.h>
#    include <windows.h>
#endif

#include <silicon/common.h>
export module silicon.scheduler:poll_win;

// poll_op 的 Windows 取值（自造常量，供 WSAPoll 之外的窗口事件封装使用）。
// 仅在 Windows 平台有内容，其余平台为空分区。

#if defined(SILICON_PLATFORM_WINDOWS)

export namespace silicon::scheduler {

enum class poll_op : uint64_t {

    read = 0x01,

    write = 0x02,

    read_write = 0x03,
};

}

#endif
