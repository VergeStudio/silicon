// task_self_deleting has been moved to pkg/silicon/task.
// This thin partition re-exports it for backward compatibility.
// NOTE: must NOT textually #include "silicon/task/detail/task_self_deleting.hpp"
// here — that header pulls in task.hpp (global-module attachment), which
// conflicts with `import silicon.task` used elsewhere in this module.
export module silicon.coroutine:detail.task_self_deleting;

import silicon.scheduler.task;

export namespace silicon::coroutine::detail {
    using silicon::scheduler::task::detail::task_self_deleting;
    using silicon::scheduler::task::detail::promise_self_deleting;
    using silicon::scheduler::task::detail::make_task_self_deleting;
} // namespace silicon::coroutine::detail
