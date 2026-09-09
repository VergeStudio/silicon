module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <Windows.h>
#    include <io.h>
#endif

#include <memory>
#include <system_error>

#include <silicon/common.h>

#include <cstdint>
module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_scheduler;

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::scheduler {

// completion 引擎的 Windows 平台接缝：错误码映射与基于 IOCP post 的唤醒通道。
// POSIX 实现见 io_scheduler_completion_unix.cpp / _linux.cpp。

bool completion_file_is_regular(int fd) {
    if(fd < 0) { return false; }
    intptr_t os_handle = ::_get_osfhandle(fd);
    if(os_handle == -1) { return false; }
    return ::GetFileType(reinterpret_cast<HANDLE>(os_handle)) == FILE_TYPE_DISK;
}

std::error_code completion_result_to_error(std::int64_t result) {
    return std::error_code(static_cast<int>(result), std::system_category());
}

class completion_wake::impl {
  public:
};

completion_wake::completion_wake(): m_p(std::make_unique<impl>()) {}

completion_wake::~completion_wake() = default;

bool completion_wake::setup(io_notifier &, void *) { return true; }

void completion_wake::teardown(io_notifier &) { }

void completion_wake::drain() noexcept { }

void completion_wake::notify(io_notifier &notifier, void *sentinel) noexcept {
    if(sentinel != nullptr) {

        notifier.post(sentinel);
    }
}

}

#endif
