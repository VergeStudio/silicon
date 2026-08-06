// task_group has been moved to pkg/silicon/task.
// This thin partition re-exports it for backward compatibility.
// NOTE: must NOT textually #include "silicon/task/task_group.hpp" here —
// that header pulls in task.hpp (global-module attachment), which conflicts
// with `import silicon.task` used elsewhere in this module.
export module silicon.coroutine:task_group;

import silicon.task;

export namespace silicon::coroutine {
    using silicon::task::task_group;
} // namespace silicon::coroutine
