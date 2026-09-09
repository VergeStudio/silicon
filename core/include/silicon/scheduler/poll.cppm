module;

#include <memory>
#include <string>

#include <silicon/common.h>
#include <cstdint>
export module silicon.scheduler:poll;

import :pipe;

// poll_op 的取值随平台不同（epoll/kqueue/Windows 常量），定义在平台分区中，
// 此处按当前平台再导出，消费方无感知。
#if defined(SILICON_PLATFORM_LINUX)
export import :poll_linux;
#elif defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)
export import :poll_kqueue;
#elif defined(SILICON_PLATFORM_WINDOWS)
export import :poll_win;
#endif

export namespace silicon::scheduler {

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

class SILICON_CORE_API poll_stop_token {
  public:
    explicit poll_stop_token(int);

    poll_stop_token(const poll_stop_token &other);

    ~poll_stop_token();

    poll_stop_token & operator=(const poll_stop_token &other) ;

    [[nodiscard]] int native_handle() const ;

  private:

    struct impl;
    std::unique_ptr<impl> m_p;
};

class SILICON_CORE_API poll_stop_source {
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

// 非导出：poll_stop_token / poll_stop_source 的 pimpl 结构（平台无关），
// 供 poll.cpp 与 poll_win.cpp / poll_unix.cpp 共享。
namespace silicon::scheduler {

struct poll_stop_token::impl {
  public:
    int m_receiver{-1};
};

struct poll_stop_source::impl {
  public:
    pipe_t m_pipe{};
};

}
