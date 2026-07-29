#pragma once

// task_self_deleting has been moved to pkg/silicon/task.
// This forwarding header keeps existing includes working.
#include "silicon/task/detail/task_self_deleting.hpp"

namespace silicon::coroutine::detail {
    using silicon::task::detail::task_self_deleting;
    using silicon::task::detail::promise_self_deleting;
    using silicon::task::detail::make_task_self_deleting;
} // namespace silicon::coroutine::detail
