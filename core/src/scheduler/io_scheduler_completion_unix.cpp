module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <sys/stat.h>
#endif

#include <silicon/common.h>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_scheduler;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::scheduler {

// POSIX 实现（linux/bsd/apple 共用）：completion_file_is_regular 平台接缝。
// Windows 实现见 io_scheduler_completion_win.cpp。

bool completion_file_is_regular(int fd) {
    if(fd < 0) { return false; }
    struct ::stat file_stat {};
    if(::fstat(fd, &file_stat) != 0) { return false; }
    return S_ISREG(file_stat.st_mode);
}

}

#endif
