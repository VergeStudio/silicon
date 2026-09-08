module;

#if defined(SILICON_PLATFORM_LINUX)
#    include <sys/epoll.h>
#endif

#include <silicon/common.h>
export module silicon.scheduler:poll_linux;

// poll_op 的 Linux/epoll 取值。仅在 Linux 平台有内容，其余平台为空分区。

#if defined(SILICON_PLATFORM_LINUX)

export namespace silicon::scheduler {

enum class poll_op : uint64_t {

    read = EPOLLIN,

    write = EPOLLOUT,

    read_write = EPOLLIN | EPOLLOUT
};

}

#endif
