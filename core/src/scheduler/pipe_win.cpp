module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <cerrno>
#    include <iostream>
#    include <system_error>
#    include <fcntl.h>
#    include <io.h>
#    include <Windows.h>
#endif

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :pipe;

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::scheduler
{

pipe_t::pipe_t(): m_p(std::make_unique<impl>())
{
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
}

pipe_t::pipe_t(const pipe_t& other): m_p(std::make_unique<impl>())
{
    m_p->m_fds[0] = _dup(other.m_p->m_fds[0]);
    m_p->m_fds[1] = _dup(other.m_p->m_fds[1]);
}

auto pipe_t::operator=(const pipe_t& other) -> pipe_t&
{
    if (std::addressof(other) != this)
    {
        m_p->m_fds[0] = _dup(other.m_p->m_fds[0]);
        m_p->m_fds[1] = _dup(other.m_p->m_fds[1]);
    }

    return *this;
}

long pipe_t::write(const void* bytes, std::size_t n)
{
    return _write(write_fd(), bytes, n);
}

long pipe_t::read(void* buffer, std::size_t n)
{
    return _read(read_fd(), buffer, n);
}

void pipe_t::close()
{
    if (m_p->m_fds[0] != -1)
    {
        ::_close(m_p->m_fds[0]);
        m_p->m_fds[0] = -1;
    }

    if (m_p->m_fds[1] != -1)
    {
        ::_close(m_p->m_fds[1]);
        m_p->m_fds[1] = -1;
    }
}

}

#endif
