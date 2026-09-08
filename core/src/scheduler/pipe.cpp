module;

#include <expected>
#include <memory>
#include <utility>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :pipe;

namespace silicon::scheduler {

// 平台无关部分：pipe_t 的公共生命周期管理。
// 平台差异（构造/复制/读写/关闭）位于 pipe_win.cpp 与 pipe_unix.cpp。

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

pipe_t::pipe_t(pipe_t&& other) noexcept: m_p(std::make_unique<impl>())
{
    m_p->m_fds = std::exchange(other.m_p->m_fds, {-1});
}

auto pipe_t::operator=(pipe_t&& other) noexcept -> pipe_t&
{
    if (std::addressof(other) != this)
    {
        m_p->m_fds = std::exchange(other.m_p->m_fds, {-1});
    }

    return *this;
}

auto pipe_t::read_fd() const -> const int&
{
    return m_p->m_fds[0];
}

auto pipe_t::write_fd() const -> const int&
{
    return m_p->m_fds[1];
}

}
