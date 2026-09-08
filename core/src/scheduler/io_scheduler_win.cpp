module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <Windows.h>
#endif

#include <silicon/common.h>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_scheduler;

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::scheduler {

// Windows 平台接缝：最近一次 OS 层错误码。POSIX 实现见 io_scheduler_unix.cpp。

int io_scheduler_last_os_error() { return static_cast<int>(::GetLastError()); }

}

#endif
