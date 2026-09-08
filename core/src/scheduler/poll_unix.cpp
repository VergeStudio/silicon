module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <unistd.h>
#endif

#include <iostream>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::scheduler {

// signal_stop 依赖平台写接口（write），POSIX 实现位于本文件；
// Windows 实现见 poll_win.cpp。

void poll_stop_source::signal_stop() {
    const int value{1};
    ssize_t written = ::write(m_p->m_pipe.write_fd(), reinterpret_cast<const void *>(&value), sizeof(value));
    if(written != sizeof(value)) {
        std::cerr << "poll::signal_stop() write failed, only wrote " << written << " bytes\n";
    }
}

}

#endif
