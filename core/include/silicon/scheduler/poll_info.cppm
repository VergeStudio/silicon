module;

#include <atomic>
#include <coroutine>
#include <map>
#include <memory>
#include <optional>
#include <utility>

#include <silicon/common.h>
export module silicon.scheduler:poll_info;

import :fd;
import :poll;
import :time;

namespace silicon::scheduler {
struct poll_info_impl;
}

export namespace silicon::scheduler {

struct CORE_API poll_info {
    using timed_events = std::multimap<silicon::scheduler::time_point, poll_info *>;

    using impl = poll_info_impl;

    poll_info();
    ~poll_info();

    poll_info(fd_t, silicon::scheduler::poll_op);
    poll_info(fd_t, silicon::scheduler::poll_op, std::optional<poll_stop_token>);

    poll_info(const poll_info &) = delete;
    poll_info(poll_info &&) = delete;
    poll_info & operator=(const poll_info &) = delete;
    poll_info & operator=(poll_info &&) = delete;

    struct poll_awaiter {
        explicit poll_awaiter(poll_info &pi) noexcept: m_pi(pi) {}

        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<>) noexcept ;
        silicon::scheduler::poll_status await_resume() noexcept ;

        poll_info &m_pi;
    };

    poll_awaiter operator co_await() noexcept { return poll_awaiter{*this}; }

    std::unique_ptr<impl> m_p;
};

}
