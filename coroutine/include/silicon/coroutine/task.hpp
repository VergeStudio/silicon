#pragma once

// task has been extracted to pkg/silicon/task as a standalone library.
// This forwarding header keeps existing includes working.
#include "silicon/task/task.hpp"

// Backward compatibility alias: task<T> is now in namespace silicon::task.
namespace silicon::coroutine {
    using silicon::task::task;
} // namespace silicon::coroutine
