module;


#include <atomic>
#include <coroutine>
#include <map>
#include <memory>
#include <optional>
#include <utility>


module silicon.scheduler;

import :poll_info;
import :poll_info_impl;

// 复用 silicon.coroutine 的基础 I/O 类型（不 export，仅本单元内简化书写）。
namespace silicon::scheduler {
using silicon::coroutine::fd_t;
using silicon::coroutine::poll_op;
using silicon::coroutine::poll_op_readable;
using silicon::coroutine::poll_op_writeable;
using silicon::coroutine::poll_status;
using silicon::coroutine::poll_stop_token;
using silicon::coroutine::time_point;
} // namespace silicon::scheduler

namespace silicon::scheduler {

poll_info::poll_info(): m_p(std::make_unique<Impl>()) {}

poll_info::~poll_info() = default;

poll_info::poll_info(fd_t fd, silicon::coroutine::poll_op op): m_p(std::make_unique<Impl>()) {
    m_p->m_fd = fd;
    m_p->m_op = op;
}

poll_info::poll_info(fd_t fd, silicon::coroutine::poll_op op, std::optional<poll_stop_token> cancel_trigger)
    : m_p(std::make_unique<Impl>()) {
    m_p->m_fd             = fd;
    m_p->m_op             = op;
    m_p->m_cancel_trigger = std::move(cancel_trigger);
}

auto poll_info::poll_awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> void {
    m_pi.m_p->m_awaiting_coroutine = awaiting_coroutine;
    std::atomic_thread_fence(std::memory_order::release);
}

auto poll_info::poll_awaiter::await_resume() noexcept -> silicon::coroutine::poll_status { return m_pi.m_p->m_poll_status; }

} // namespace silicon::scheduler
