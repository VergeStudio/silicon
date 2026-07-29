module;

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

module silicon.coroutine;

#if defined(_WIN32)
#    include <io.h>
#    include <fcntl.h>
#    include <Windows.h>
#else
#    include <fcntl.h>
#    include <unistd.h>
#endif

namespace silicon::coroutine::detail
{

pipe_t::pipe_t()
{
    // Using pipe instead of pipe2 since macos does not have support for pipe2.
#if defined(_WIN32)
    if (_pipe(m_fds.data(), 256, _O_BINARY) != 0)
    {
        const std::string msg = "Failed to create pipe, errno=[" + std::string{std::strerror(errno)} + "]";
        throw std::runtime_error(msg);
    }

    // Set the pipe file descriptors to be non-blocking.
    for (const auto& fd : m_fds)
    {
        HANDLE hPipe = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
        DWORD mode = PIPE_NOWAIT;
        SetNamedPipeHandleState(hPipe, &mode, nullptr, nullptr);
    }
#else
    if (::pipe(m_fds.data()) != 0)
    {
        const std::string msg = "Failed to create pipe, errno=[" + std::string{std::strerror(errno)} + "]";
        throw std::runtime_error(msg);
    }

    // Set the pipe file descriptors to be non-blocking.
    for (const auto& fd : m_fds)
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

pipe_t::pipe_t(const pipe_t& other)
{
#if defined(_WIN32)
    m_fds[0] = _dup(other.m_fds[0]);
    m_fds[1] = _dup(other.m_fds[1]);
#else
    m_fds[0] = dup(other.m_fds[0]);
    m_fds[1] = dup(other.m_fds[1]);
#endif
}

pipe_t::pipe_t(pipe_t&& other) noexcept
{
    m_fds = std::exchange(other.m_fds, {-1});
}

auto pipe_t::operator=(const pipe_t& other) -> pipe_t&
{
    if (std::addressof(other) != this)
    {
#if defined(_WIN32)
        m_fds[0] = _dup(other.m_fds[0]);
        m_fds[1] = _dup(other.m_fds[1]);
#else
        m_fds[0] = dup(other.m_fds[0]);
        m_fds[1] = dup(other.m_fds[1]);
#endif
    }

    return *this;
}

auto pipe_t::operator=(pipe_t&& other) noexcept -> pipe_t&
{
    if (std::addressof(other) != this)
    {
        m_fds = std::exchange(other.m_fds, {-1});
    }

    return *this;
}

auto pipe_t::write(const void* bytes, std::size_t n) -> long
{
#if defined(_WIN32)
    return _write(write_fd(), bytes, n);
#else
    return ::write(write_fd(), bytes, n);
#endif
}

auto pipe_t::read(void* buffer, std::size_t n) -> long
{
#if defined(_WIN32)
    return _read(read_fd(), buffer, n);
#else
    return ::read(read_fd(), buffer, n);
#endif
}

auto pipe_t::read_fd() const -> const fd_t&
{
    return m_fds[0];
}

auto pipe_t::write_fd() const -> const fd_t&
{
    return m_fds[1];
}

auto pipe_t::close() -> void
{
    if (m_fds[0] != -1)
    {
#if defined(_WIN32)
        ::_close(m_fds[0]);
#else
        ::close(m_fds[0]);
#endif
        m_fds[0] = -1;
    }

    if (m_fds[1] != -1)
    {
#if defined(_WIN32)
        ::_close(m_fds[1]);
#else
        ::close(m_fds[1]);
#endif
        m_fds[1] = -1;
    }
}

} // namespace silicon::coroutine::detail
