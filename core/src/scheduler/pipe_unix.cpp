module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <cerrno>
#    include <iostream>
#    include <system_error>
#    include <fcntl.h>
#    include <unistd.h>
#endif

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :pipe;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::scheduler
{

pipe_t::pipe_t(): m_p(std::make_unique<impl>())
{
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
}

pipe_t::pipe_t(const pipe_t& other): m_p(std::make_unique<impl>())
{
    m_p->m_fds[0] = dup(other.m_p->m_fds[0]);
    m_p->m_fds[1] = dup(other.m_p->m_fds[1]);
}

auto pipe_t::operator=(const pipe_t& other) -> pipe_t&
{
    if (std::addressof(other) != this)
    {
        m_p->m_fds[0] = dup(other.m_p->m_fds[0]);
        m_p->m_fds[1] = dup(other.m_p->m_fds[1]);
    }

    return *this;
}

long pipe_t::write(const void* bytes, std::size_t n)
{
    return ::write(write_fd(), bytes, n);
}

long pipe_t::read(void* buffer, std::size_t n)
{
    return ::read(read_fd(), buffer, n);
}

void pipe_t::close()
{
    if (m_p->m_fds[0] != -1)
    {
        ::close(m_p->m_fds[0]);
        m_p->m_fds[0] = -1;
    }

    if (m_p->m_fds[1] != -1)
    {
        ::close(m_p->m_fds[1]);
        m_p->m_fds[1] = -1;
    }
}

}

#endif
