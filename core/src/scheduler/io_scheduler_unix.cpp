module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <cerrno>
#endif

#include <silicon/common.h>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_scheduler;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::scheduler {

// POSIX 平台接缝：最近一次 OS 层错误码。Windows 实现见 io_scheduler_win.cpp。

int io_scheduler_last_os_error() { return errno; }

}

#endif
