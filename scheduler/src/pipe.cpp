module;

#include <cerrno>
#include <cstring>
#include <memory>
#include <system_error> // std::system_category：替代被 MSVC 弃用的 strerror
#include <stdexcept>
#include <string>
#include <utility>
#include <coroutine>
#include <map>
#include <optional>

// 平台头必须置于全局模块片段（module 声明之前）；
// 在 module purview 内文本包含会与 BMI 中的声明产生附着冲突。
#if defined(SILICON_PLATFORM_WINDOWS)
#    include <io.h>
#    include <fcntl.h>
#    include <Windows.h>
#else
#    include <fcntl.h>
#    include <unistd.h>
#endif

module silicon.scheduler;
#include "poll_info_impl.hpp"

namespace silicon::coroutine
{

class pipe_t::impl {
  public:
    std::array<fd_t, 2> m_fds{-1};
};

pipe_t::pipe_t(): m_p(std::make_unique<impl>())
{
    // Using pipe instead of pipe2 since macos does not have support for pipe2.
#if defined(SILICON_PLATFORM_WINDOWS)
    if (_pipe(m_p->m_fds.data(), 256, _O_BINARY) != 0)
    {
        const std::string msg = "Failed to create pipe, errno=[" + std::system_category().message(errno) + "]";
        throw std::runtime_error(msg);
    }

    // Set the pipe file descriptors to be non-blocking.
    for (const auto& fd : m_p->m_fds)
    {
        HANDLE hPipe = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
        DWORD mode = PIPE_NOWAIT;
        SetNamedPipeHandleState(hPipe, &mode, nullptr, nullptr);
    }
#else
    if (::pipe(m_p->m_fds.data()) != 0)
    {
        const std::string msg = "Failed to create pipe, errno=[" + std::system_category().message(errno) + "]";
        throw std::runtime_error(msg);
    }

    // Set the pipe file descriptors to be non-blocking.
    for (const auto& fd : m_p->m_fds)
    {
        int flags = fcntl(fd, F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);
    }
#endif
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

auto pipe_t::write(const void* bytes, std::size_t n) -> long
{
#if defined(SILICON_PLATFORM_WINDOWS)
    return _write(write_fd(), bytes, n);
#else
    return ::write(write_fd(), bytes, n);
#endif
}

auto pipe_t::read(void* buffer, std::size_t n) -> long
{
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

auto pipe_t::close() -> void
{
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

} // namespace silicon::coroutine
