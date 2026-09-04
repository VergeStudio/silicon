module;


#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <exception>
#include <coroutine>
#include <map>
#include <optional>

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <io.h>
#else
#    include <unistd.h>
#endif

module silicon.scheduler;

import :poll_info_impl;


namespace silicon::scheduler {

static const std::string poll_unknown{"unknown"};

static const std::string poll_op_read{"read"};
static const std::string poll_op_write{"write"};
static const std::string poll_op_read_write{"read_write"};

auto to_string(poll_op op) -> const std::string & {
    switch(op) {
        case poll_op::read:
            return poll_op_read;
        case poll_op::write:
            return poll_op_write;
        case poll_op::read_write:
            return poll_op_read_write;
        default:
            return poll_unknown;
    }
}

static const std::string poll_status_read{"read"};
static const std::string poll_status_write{"write"};
static const std::string poll_status_timeout{"timeout"};
static const std::string poll_status_error{"error"};
static const std::string poll_status_closed{"closed"};

auto to_string(poll_status status) -> const std::string & {
    switch(status) {
        case poll_status::read:
            return poll_status_read;
        case poll_status::write:
            return poll_status_write;
        case poll_status::timeout:
            return poll_status_timeout;
        case poll_status::error:
            return poll_status_error;
        case poll_status::closed:
            return poll_status_closed;
        default:
            return poll_unknown;
    }
}






struct poll_stop_token::impl {
  public:
    fd_t m_receiver{-1};
};

poll_stop_token::poll_stop_token(fd_t receiver): m_p(std::make_unique<impl>()) {
    m_p->m_receiver = receiver;
}

poll_stop_token::poll_stop_token(const poll_stop_token &other): m_p(std::make_unique<impl>()) {
    m_p->m_receiver = other.m_p->m_receiver;
}

poll_stop_token::~poll_stop_token() = default;

auto poll_stop_token::operator=(const poll_stop_token &other) -> poll_stop_token & {
    if(std::addressof(other) != this) {
        m_p->m_receiver = other.m_p->m_receiver;
    }
    return *this;
}

auto poll_stop_token::native_handle() const -> fd_t {
    return m_p->m_receiver;
}






struct poll_stop_source::impl {
  public:
    pipe_t m_pipe{};
};

poll_stop_source::poll_stop_source(): m_p(std::make_unique<impl>()) {

    if(!m_p->m_pipe.is_valid()) {
        std::terminate();
    }
}

poll_stop_source::poll_stop_source(poll_stop_source &&other) noexcept: m_p(std::make_unique<impl>()) {
    *this = std::move(other);
}

poll_stop_source::~poll_stop_source() = default;

auto poll_stop_source::operator=(poll_stop_source &&other) -> poll_stop_source & {
    if(std::addressof(other) != this) {
        m_p->m_pipe = std::move(other.m_p->m_pipe);
    }
    return *this;
}

auto poll_stop_source::get_token() const -> poll_stop_token {
    return poll_stop_token(m_p->m_pipe.read_fd());
}

void poll_stop_source::signal_stop() {
    const int value{1};
#if defined(SILICON_PLATFORM_WINDOWS)
    int written = ::_write(m_p->m_pipe.write_fd(), reinterpret_cast<const void *>(&value), sizeof(value));
#else
    ssize_t written = ::write(m_p->m_pipe.write_fd(), reinterpret_cast<const void *>(&value), sizeof(value));
#endif
    if(written != sizeof(value)) {
        std::cerr << "poll::signal_stop() write failed, only wrote " << written << " bytes\n";
    }
}

}
