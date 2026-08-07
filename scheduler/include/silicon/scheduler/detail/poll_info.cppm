module;


#include <atomic>
#include <coroutine>
#include <map>
#include <memory>
#include <optional>
#include <utility>


export module silicon.scheduler:detail.poll_info;

import :fd;
import :poll;
import :time;

// 基础 I/O 类型已随调度原语迁入本模块（命名空间仍为 silicon::coroutine），
// 此处仅在本单元内引入简化书写，不 export。
namespace silicon::scheduler {
using silicon::coroutine::fd_t;
using silicon::coroutine::poll_op;
using silicon::coroutine::poll_op_readable;
using silicon::coroutine::poll_op_writeable;
using silicon::coroutine::poll_status;
using silicon::coroutine::poll_stop_token;
using silicon::coroutine::time_point;
} // namespace silicon::scheduler

export namespace silicon::scheduler::detail {

/**
 * Poll Info encapsulates everything about a poll operation for the event as well as its paired
 * timeout.  This is important since coroutines that are waiting on an event or timeout do not
 * immediately execute, they are re-scheduled onto the thread pool, so its possible its pair
 * event or timeout also triggers while the coroutine is still waiting to resume.  This means that
 * the first one to happen, the event itself or its timeout, needs to disable the other pair item
 * prior to resuming the coroutine.
 *
 * Finally, its also important to note that the event and its paired timeout could happen during
 * the same epoll_wait and possibly trigger the coroutine to start twice.  Only one can win, so the
 * first one processed sets m_processed to true and any subsequent events in the same epoll batch
 * are effectively discarded.
 *
 * The implementation state (`Impl`) is defined in the non-exported partition
 * `silicon.scheduler:detail.poll_info_impl`; its full definition is therefore invisible outside
 * `silicon.scheduler`.  Members that touch `Impl` are declared here and defined out-of-line in
 * `scheduler/src/detail/poll_info.cpp`.
 */
struct poll_info {
    using timed_events = std::multimap<silicon::coroutine::time_point, detail::poll_info *>;

    /// Implementation state of a poll operation.  Kept behind `m_p` so the layout of a poll
    /// operation is an implementation detail.  The full definition lives in the non-exported
    /// `silicon.scheduler:detail.poll_info_impl` partition.
    struct Impl;

    poll_info();
    ~poll_info();

    poll_info(fd_t fd, silicon::coroutine::poll_op op);
    poll_info(fd_t fd, silicon::coroutine::poll_op op, std::optional<poll_stop_token> cancel_trigger);

    poll_info(const poll_info &) = delete;
    poll_info(poll_info &&) = delete;
    auto operator=(const poll_info &) -> poll_info & = delete;
    auto operator=(poll_info &&) -> poll_info & = delete;

    struct poll_awaiter {
        explicit poll_awaiter(poll_info &pi) noexcept: m_pi(pi) {}

        auto await_ready() const noexcept -> bool { return false; }
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> void;
        auto await_resume() noexcept -> silicon::coroutine::poll_status;

        poll_info &m_pi;
    };

    auto operator co_await() noexcept -> poll_awaiter { return poll_awaiter{*this}; }

    std::unique_ptr<Impl> m_p;
};

} // namespace silicon::scheduler::detail
