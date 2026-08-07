module;


#include <atomic>
#include <coroutine>
#include <map>
#include <memory>
#include <optional>
#include <utility>


export module silicon.scheduler:detail.poll_info_impl;

import :fd;
import :poll;
import :time;
import :detail.poll_info;

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

namespace silicon::scheduler::detail {

/**
 * Full definition of `poll_info::Impl`, kept in a non-exported module partition so the
 * implementation state of a poll operation is invisible outside `silicon.scheduler`.
 *
 * The interface unit (`silicon.scheduler:detail.poll_info`) only forward-declares `Impl` and
 * declares the members that touch it out-of-line; the out-of-line definitions live in
 * `scheduler/src/detail/poll_info.cpp`.  Backend implementation units that dereference
 * `poll_info::Impl` import this partition directly.
 */
struct poll_info::Impl {
  public:
    /// The file descriptor being polled on.  This is needed so that if the timeout occurs first
    /// then the event loop can immediately disable the event within epoll.
    fd_t m_fd{-1};
    /// The operation that is being waited for to be performed on the file descriptor.
    silicon::coroutine::poll_op m_op{};
    /// The timeout's position in the timeout map.  A poll() with no timeout or yield() this is
    /// empty.  This is needed so that if the event occurs first then the event loop can
    /// immediately disable the timeout within epoll.
    std::optional<poll_info::timed_events::iterator> m_timer_pos{std::nullopt};
    /// The awaiting coroutine for this poll info to resume upon event or timeout.
    std::coroutine_handle<> m_awaiting_coroutine;
    /// The status of the poll operation.
    silicon::coroutine::poll_status m_poll_status{silicon::coroutine::poll_status::error};
    /// Did the timeout and event trigger at the same time on the same epoll_wait call?
    /// Once this is set to true all future events on this poll info are null and void.
    bool m_processed{false};
    /// Cancellation receiver of this poll operation.
    std::optional<poll_stop_token> m_cancel_trigger{std::nullopt};
};

} // namespace silicon::scheduler::detail
