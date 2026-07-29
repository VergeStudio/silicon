#pragma once

// task_group has been moved to pkg/silicon/task.
// This forwarding header keeps existing includes working.
#include "silicon/task/task_group.hpp"

namespace silicon::coroutine {
    using silicon::task::task_group;
} // namespace silicon::coroutine
