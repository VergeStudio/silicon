module;

#include <utility>
#include <memory>

#include <array>
#include <iostream>
#include <string>

#if defined(SILICON_PLATFORM_LINUX)
#    include <sys/epoll.h>
#elif defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)
#    include <sys/event.h>
#elif defined(SILICON_PLATFORM_WINDOWS)
#    include <winsock2.h>
#    include <windows.h>
#    include <io.h>
#endif
#if !defined(SILICON_PLATFORM_WINDOWS)
#    include <unistd.h>
#endif

#include <silicon/common.h>
export module silicon.scheduler:poll;

import :pipe;
import :fd;

export namespace silicon::scheduler {
#if defined(SILICON_PLATFORM_LINUX)
enum class poll_op : uint64_t {

    read = EPOLLIN,

    write = EPOLLOUT,

    read_write = EPOLLIN | EPOLLOUT
};
#elif defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)
enum class poll_op : int64_t {

    read = EVFILT_READ,

    write = EVFILT_WRITE,

    read_write = -5,
};
#elif defined(SILICON_PLATFORM_WINDOWS)
enum class poll_op : uint64_t {

    read = 0x01,

    write = 0x02,

    read_write = 0x03,
};
#endif

inline bool poll_op_readable(poll_op op) {
    return (static_cast<uint64_t>(op) & static_cast<uint64_t>(poll_op::read));
}

inline bool poll_op_writeable(poll_op op) {
    return (static_cast<uint64_t>(op) & static_cast<uint64_t>(poll_op::write));
}

auto to_string(poll_op) -> const std::string &;

enum class poll_status {

    read,

    write,

    timeout,

    error,

    closed,

    cancelled,
};

auto to_string(poll_status) -> const std::string &;

class CORE_API poll_stop_token {
  public:
    explicit poll_stop_token(fd_t);

    poll_stop_token(const poll_stop_token &other);

    ~poll_stop_token();

    poll_stop_token & operator=(const poll_stop_token &other) ;

    [[nodiscard]] fd_t native_handle() const ;

  private:

    struct impl;
    std::unique_ptr<impl> m_p;
};

class CORE_API poll_stop_source {
  public:
    poll_stop_source();

    poll_stop_source(const poll_stop_source &) = delete;
    poll_stop_source(poll_stop_source &&other) noexcept;

    ~poll_stop_source();

    poll_stop_source & operator=(const poll_stop_source &) = delete;
    poll_stop_source & operator=(poll_stop_source &&other) ;

    [[nodiscard]] poll_stop_token get_token() const ;

    void signal_stop() ;

  private:

    struct impl;
    std::unique_ptr<impl> m_p;
};

}
