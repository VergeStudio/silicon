module;

#if defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)
#    include <sys/event.h>
#endif

#include <silicon/common.h>
#include <cstdint>
export module silicon.scheduler:poll_kqueue;

// poll_op 的 kqueue 取值（BSD/macOS）。仅在 BSD/Apple 平台有内容，其余平台为空分区。

#if defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)

export namespace silicon::scheduler {

enum class poll_op : int64_t {

    read = EVFILT_READ,

    write = EVFILT_WRITE,

    read_write = -5,
};

}

#endif
