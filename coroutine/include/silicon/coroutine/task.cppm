// task has been extracted to pkg/silicon/task as a standalone library.
// This thin partition re-exports it for backward compatibility.
// NOTE: must NOT textually #include "silicon/task/task.hpp" here —
// coroutine.cppm imports silicon.task (module attachment), and mixing
// textual inclusion of the same entities (global-module attachment)
// causes "declaration attached to named module cannot be attached to
// other modules" errors.
export module silicon.coroutine:task;

export import silicon.scheduler.task;

// Backward compatibility alias: task<T> is now in namespace silicon::task.
export namespace silicon::coroutine {
    using silicon::scheduler::task::task;
} // namespace silicon::coroutine
