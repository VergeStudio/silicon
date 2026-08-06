// pool has been moved to pkg/silicon/thread as a sub-module.
// This thin partition re-exports it for backward compatibility.
// NOTE: must NOT textually #include "silicon/thread/pool.hpp" here —
// that header pulls in task.hpp (global-module attachment), which conflicts
// with `import silicon.task` used elsewhere in this module.
export module silicon.coroutine:pool;

import silicon.thread.pool;

export namespace silicon::coroutine {
    using silicon::thread::pool;
} // namespace silicon::coroutine
