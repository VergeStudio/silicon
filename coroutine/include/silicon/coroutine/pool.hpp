#pragma once

// pool has been moved to pkg/silicon/thread as a sub-module.
// The module is now silicon.thread. This forwarding header keeps existing includes working.
#include "silicon/thread/pool.hpp"

namespace silicon::coroutine {
    using silicon::thread::pool;
} // namespace silicon::coroutine
