module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <iostream>
#    include <io.h>
#endif

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll;

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::scheduler {

// signal_stop 依赖平台写接口（_write），Windows 实现位于本文件；
// POSIX 实现见 poll_unix.cpp。

void poll_stop_source::signal_stop() {
    const int value{1};
    int written = ::_write(m_p->m_pipe.write_fd(), reinterpret_cast<const void *>(&value), sizeof(value));
    if(written != sizeof(value)) {
        std::cerr << "poll::signal_stop() write failed, only wrote " << written << " bytes\n";
    }
}

}

#endif
