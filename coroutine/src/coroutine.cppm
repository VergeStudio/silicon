module;

// ============================================================================
// Public headers — included in the global module fragment
// ============================================================================

// Concepts
#include "silicon/coroutine/concepts/awaitable.hpp"
#include "silicon/coroutine/concepts/buffer.hpp"
#include "silicon/coroutine/concepts/executor.hpp"
#include "silicon/coroutine/concepts/promise.hpp"
#include "silicon/coroutine/concepts/range_of.hpp"

// Core utility types
#include "silicon/coroutine/common.h"
#include "silicon/coroutine/expected.hpp"
#include "silicon/coroutine/detail/void_value.hpp"

// Coroutine primitives
#include "silicon/coroutine/generator.hpp"
#include "silicon/coroutine/sync_wait.hpp"
#include "silicon/coroutine/when_all.hpp"
#include "silicon/coroutine/when_any.hpp"
#include "silicon/coroutine/invoke.hpp"
#include "silicon/coroutine/time.hpp"
#include "silicon/coroutine/attribute.hpp"

// Sync primitives
#include "silicon/coroutine/mutex.hpp"
#include "silicon/coroutine/shared_mutex.hpp"
#include "silicon/coroutine/event.hpp"
#include "silicon/coroutine/semaphore.hpp"
#include "silicon/coroutine/condition_variable.hpp"
#include "silicon/coroutine/latch.hpp"
#include "silicon/coroutine/queue.hpp"
#include "silicon/coroutine/ring_buffer.hpp"

// Task management (still owned by coroutine after extraction)
#include "silicon/coroutine/task_container.hpp"
#include "silicon/coroutine/task_group.hpp"

// Scheduling & IO
#include "silicon/coroutine/scheduler.hpp"
#include "silicon/coroutine/default_executor.hpp"
#include "silicon/coroutine/io_notifier.hpp"
#include "silicon/coroutine/poll.hpp"
#include "silicon/coroutine/fd.hpp"

// Detail
#include "silicon/coroutine/detail/awaiter_list.hpp"
#include "silicon/coroutine/detail/task_self_deleting.hpp"
#include "silicon/coroutine/detail/timer_handle.hpp"
#include "silicon/coroutine/detail/pipe.hpp"
#include "silicon/coroutine/detail/poll_info.hpp"
#if defined(_WIN32)
#    include "silicon/coroutine/detail/io_notifier_iocp.hpp"
#elif defined(__linux__)
#    include "silicon/coroutine/detail/io_notifier_epoll.hpp"
#elif defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__)
#    include "silicon/coroutine/detail/io_notifier_kqueue.hpp"
#endif

// ============================================================================
// Module interface
// ============================================================================

export module silicon.coroutine;

import silicon.task;

export import :config;

// ---------------------------------------------------------------------------
// Re-export public types via using-declarations so module consumers can
// access them through `import silicon.coroutine;` without extra includes.
// ---------------------------------------------------------------------------

export namespace silicon::coroutine::concepts {
    using ::silicon::coroutine::concepts::awaitable;
    using ::silicon::coroutine::concepts::buffer;
    using ::silicon::coroutine::concepts::executor;
    using ::silicon::coroutine::concepts::promise;
    using ::silicon::coroutine::concepts::range_of;
} // namespace silicon::coroutine::concepts

export namespace silicon::coroutine {
    // Coroutine primitives (imported from silicon.task)
    using ::silicon::task::task;

    // Core utility types
    using ::silicon::coroutine::expected;
    using ::silicon::coroutine::unexpected;

    // Coroutine primitives
    using ::silicon::coroutine::generator;
    using ::silicon::coroutine::sync_wait;
    using ::silicon::coroutine::when_all_ready;
    using ::silicon::coroutine::when_any_ready;
    using ::silicon::coroutine::invoke;
    using ::silicon::coroutine::attribute;

    // Sync primitives
    using ::silicon::coroutine::mutex;
    using ::silicon::coroutine::shared_mutex;
    using ::silicon::coroutine::event;
    using ::silicon::coroutine::semaphore;
    using ::silicon::coroutine::condition_variable;
    using ::silicon::coroutine::latch;
    using ::silicon::coroutine::queue;
    using ::silicon::coroutine::ring_buffer;

    // Task management
    using ::silicon::coroutine::task_container;
    using ::silicon::coroutine::task_group;

    // Scheduling & IO
    using ::silicon::coroutine::scheduler;
    using ::silicon::coroutine::io_notifier;
    using ::silicon::coroutine::poll;
    using ::silicon::coroutine::fd;
    using ::silicon::coroutine::poll_result;

    // Default executor
    using ::silicon::coroutine::default_executor::executor;
    using ::silicon::coroutine::default_executor::set_executor_options;
    using ::silicon::coroutine::default_executor::io_executor;
    using ::silicon::coroutine::default_executor::set_io_executor_options;
} // namespace silicon::coroutine

// ============================================================================
// Detail namespace — NOT exported; consumers who need internal types should
// #include the relevant detail header directly.
// ============================================================================

namespace silicon::coroutine::detail {
    using ::silicon::coroutine::detail::awaiter_list;
    using ::silicon::coroutine::detail::make_task_self_deleting;
    using ::silicon::coroutine::detail::task_self_deleting;
    using ::silicon::coroutine::detail::timer_handle;
    using ::silicon::coroutine::detail::void_value;
} // namespace silicon::coroutine::detail
