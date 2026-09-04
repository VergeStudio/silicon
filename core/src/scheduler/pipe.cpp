module;

#include <cerrno>
#include <cstring>
#include <expected>
#include <iostream>
#include <memory>
#include <system_error>
#include <stdexcept>
#include <string>
#include <utility>
#include <coroutine>
#include <map>
#include <optional>

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <io.h>
#    include <fcntl.h>
#    include <Windows.h>
#else
#    include <fcntl.h>
#    include <unistd.h>
#endif

module silicon.scheduler;
import silicon.scheduler.error;
import :poll_info_impl;

namespace silicon::scheduler
{

class pipe_t::impl {
  public:
    std::array<fd_t, 2> m_fds{-1};
};

pipe_t::pipe_t(): m_p(std::make_unique<impl>())
{

#if defined(SILICON_PLATFORM_WINDOWS)
    if (_pipe(m_p->m_fds.data(), 256, _O_BINARY) != 0)
    {
        std::cerr << "Failed to create pipe, errno=[" << std::system_category().message(errno) << "]\n";
        return;
    }

    for (const auto& fd : m_p->m_fds)
    {
        HANDLE hPipe = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
        DWORD mode = PIPE_NOWAIT;
        SetNamedPipeHandleState(hPipe, &mode, nullptr, nullptr);
    }
#else
    if (::pipe(m_p->m_fds.data()) != 0)
    {
        std::cerr << "Failed to create pipe, errno=[" << std::system_category().message(errno) << "]\n";
        return;
    }

    for (const auto& fd : m_p->m_fds)
    {
        int flags = fcntl(fd, F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);
    }
#endif
}

bool pipe_t::is_valid() const noexcept
{
    return m_p != nullptr && m_p->m_fds[0] != -1 && m_p->m_fds[1] != -1;
}

std::expected<pipe_t, std::error_code> pipe_t::create()
{
    pipe_t p;
    if(!p.is_valid())
    {
        return std::unexpected(silicon::scheduler::make_error_code(silicon::scheduler::scheduler_error::kPipeCreateFailed));
    }
    return p;
}

pipe_t::~pipe_t()
{
    close();
}

pipe_t::pipe_t(const pipe_t& other): m_p(std::make_unique<impl>())
{
#if defined(SILICON_PLATFORM_WINDOWS)
    m_p->m_fds[0] = _dup(other.m_p->m_fds[0]);
    m_p->m_fds[1] = _dup(other.m_p->m_fds[1]);
#else
    m_p->m_fds[0] = dup(other.m_p->m_fds[0]);
    m_p->m_fds[1] = dup(other.m_p->m_fds[1]);
#endif
}

pipe_t::pipe_t(pipe_t&& other) noexcept: m_p(std::make_unique<impl>())
{
    m_p->m_fds = std::exchange(other.m_p->m_fds, {-1});
}

auto pipe_t::operator=(const pipe_t& other) -> pipe_t&
{
    if (std::addressof(other) != this)
    {
#if defined(SILICON_PLATFORM_WINDOWS)
        m_p->m_fds[0] = _dup(other.m_p->m_fds[0]);
        m_p->m_fds[1] = _dup(other.m_p->m_fds[1]);
#else
        m_p->m_fds[0] = dup(other.m_p->m_fds[0]);
        m_p->m_fds[1] = dup(other.m_p->m_fds[1]);
#endif
    }

    return *this;
}

auto pipe_t::operator=(pipe_t&& other) noexcept -> pipe_t&
{
    if (std::addressof(other) != this)
    {
        m_p->m_fds = std::exchange(other.m_p->m_fds, {-1});
    }

    return *this;
}

long pipe_t::write(const void* bytes, std::size_t n) {
#if defined(SILICON_PLATFORM_WINDOWS)
    return _write(write_fd(), bytes, n);
#else
    return ::write(write_fd(), bytes, n);
#endif
}

long pipe_t::read(void* buffer, std::size_t n) {
#if defined(SILICON_PLATFORM_WINDOWS)
    return _read(read_fd(), buffer, n);
#else
    return ::read(read_fd(), buffer, n);
#endif
}

auto pipe_t::read_fd() const -> const fd_t&
{
    return m_p->m_fds[0];
}

auto pipe_t::write_fd() const -> const fd_t&
{
    return m_p->m_fds[1];
}

void pipe_t::close() {
    if (m_p->m_fds[0] != -1)
    {
#if defined(SILICON_PLATFORM_WINDOWS)
        ::_close(m_p->m_fds[0]);
#else
        ::close(m_p->m_fds[0]);
#endif
        m_p->m_fds[0] = -1;
    }

    if (m_p->m_fds[1] != -1)
    {
#if defined(SILICON_PLATFORM_WINDOWS)
        ::_close(m_p->m_fds[1]);
#else
        ::close(m_p->m_fds[1]);
#endif
        m_p->m_fds[1] = -1;
    }
}

}
