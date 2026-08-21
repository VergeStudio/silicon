module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
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
#    include <io.h> // ::_write
#endif
#if !defined(SILICON_PLATFORM_WINDOWS)
#    include <unistd.h>
#endif


export module silicon.scheduler:poll;

import :pipe;
import :fd;

export namespace silicon::coroutine {
#if defined(SILICON_PLATFORM_LINUX)
enum class poll_op : uint64_t {
    /// Poll for read operations.
    read = EPOLLIN,
    /// Poll for write operations.
    write = EPOLLOUT,
    /// Poll for read and write operations.
    read_write = EPOLLIN | EPOLLOUT
};
#elif defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)
enum class poll_op : int64_t {
    /// Poll for read operations.
    read = EVFILT_READ,
    /// Poll for write operations.
    write = EVFILT_WRITE,
    /// Poll for read and write operations.
    read_write = -5,
};
#elif defined(SILICON_PLATFORM_WINDOWS)
enum class poll_op : uint64_t {
    /// Poll for read operations.
    read = 0x01,
    /// Poll for write operations.
    write = 0x02,
    /// Poll for read and write operations.
    read_write = 0x03,
};
#endif

inline bool poll_op_readable(poll_op op) {
    return (static_cast<uint64_t>(op) & static_cast<uint64_t>(poll_op::read));
}

inline bool poll_op_writeable(poll_op op) {
    return (static_cast<uint64_t>(op) & static_cast<uint64_t>(poll_op::write));
}

auto to_string(poll_op op) -> const std::string &;

enum class poll_status {
    /// The poll operation was was successful with a read-event.
    read,
    /// The poll operation was was successful with a write-event.
    write,
    /// The poll operation timed out.
    timeout,
    /// The file descriptor had an error while polling.
    error,
    /// The file descriptor has been closed by the remote or an internal error/close.
    closed,
    /// The poll operation was cancelled by a 'poll_stop_source'.
    cancelled,
};

auto to_string(poll_status status) -> const std::string &;

class poll_stop_token {
  public:
    explicit poll_stop_token(fd_t);

    // poll_stop_token is logically a value (wraps a single fd), so keep it copyable
    // by cloning the underlying int rather than deleting copy (which would force a
    // move-only cascade through std::optional<poll_stop_token> users).
    poll_stop_token(const poll_stop_token &other);

    ~poll_stop_token();

    poll_stop_token & operator=(const poll_stop_token &other) ;

    [[nodiscard]] fd_t native_handle() const ;

  private:
    /// Implementation state, fully hidden in the implementation unit.
    struct impl;
    std::unique_ptr<impl> m_p;
};

class poll_stop_source {
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
    /// Implementation state, fully hidden in the implementation unit.
    struct impl;
    std::unique_ptr<impl> m_p;
};

} // namespace silicon::coroutine
