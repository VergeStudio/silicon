#pragma once

#include <chrono>

namespace silicon::coroutine {
using clock = std::chrono::steady_clock;
using time_point = clock::time_point;
} // namespace silicon::coroutine
