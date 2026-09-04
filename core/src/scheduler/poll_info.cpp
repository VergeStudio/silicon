module;


#include <atomic>
#include <coroutine>
#include <map>
#include <memory>
#include <optional>
#include <utility>


module silicon.scheduler;
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫。
#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll_info;
import :poll_info_impl;

namespace silicon::scheduler {

poll_info::poll_info(): m_p(std::make_unique<impl>()) {}

poll_info::~poll_info() = default;

poll_info::poll_info(fd_t fd, silicon::scheduler::poll_op op): m_p(std::make_unique<impl>()) {
    m_p->m_fd = fd;
    m_p->m_op = op;
}

poll_info::poll_info(fd_t fd, silicon::scheduler::poll_op op, std::optional<poll_stop_token> cancel_trigger)
    : m_p(std::make_unique<impl>()) {
    m_p->m_fd             = fd;
    m_p->m_op             = op;
    m_p->m_cancel_trigger = std::move(cancel_trigger);
}

void poll_info::poll_awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    m_pi.m_p->m_awaiting_coroutine = awaiting_coroutine;
    std::atomic_thread_fence(std::memory_order::release);
}

silicon::scheduler::poll_status poll_info::poll_awaiter::await_resume() noexcept { return m_pi.m_p->m_poll_status; }

} // namespace silicon::scheduler
