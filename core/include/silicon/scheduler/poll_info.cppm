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

// poll_info 的 PIMPL 实现类型：命名空间作用域的前置声明，**刻意不 export**。
//
// 为什么它不是 `poll_info` 的嵌套类 `struct impl { ... }`：MSVC 目前不把「外围类
// 所在模块单元之外给出的嵌套类定义」写进 IFC，因此 `export struct poll_info::impl
// {...}` 放在另一个分区时，即使实现单元写了 `import :poll_info_impl;`，编译器也只
// 能看到这里的前置声明并报 C2027（已用最小用例在 MSVC 14.44 上复现）。
// 故实现类型改为命名空间作用域的 `poll_info_impl`，再由 `poll_info::impl` 以别名
// 形式暴露名称——对既有实现代码（`m_p->m_fd` 等）完全透明。
//
// 完整定义位于分区 `silicon.scheduler:poll_info_impl`，该分区不被主模块接口单元
// `export import`，因此 `import silicon.scheduler;` 的消费方只能拿到不完整类型，
// PIMPL 封装不受影响。
namespace silicon::scheduler {
struct poll_info_impl;
} // namespace silicon::scheduler

export namespace silicon::scheduler {

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
 * The implementation state is fully defined in the partition
 * `silicon.scheduler:poll_info_impl`, which the primary module interface unit
 * (`scheduler.cppm`) imports but does **not** `export import`.  The definition
 * is therefore reachable from in-module implementation units that write
 * `import :poll_info_impl;` while remaining invisible to module consumers.
 * Members that touch the state are declared here and defined out-of-line in
 * `core/src/scheduler/poll_info.cpp`.
 */
struct CORE_API poll_info {
    using timed_events = std::multimap<silicon::scheduler::time_point, poll_info *>;

    /// Implementation state of a poll operation.  Kept behind `m_p` so the layout of a poll
    /// operation is an implementation detail.  This is an alias for the namespace-scope
    /// `silicon::scheduler::poll_info_impl` (see the note above it for why it cannot be a
    /// nested class): the aliased type is never exported, so consumers of
    /// `silicon.scheduler` only ever see an incomplete type.
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

} // namespace silicon::scheduler
