module;

#include <chrono>

export module silicon.scheduler:time;

import silicon.time;

export namespace silicon::scheduler {
// 仅作为 core/time 的薄别名，统一时间来源（详见 core/src/time/specs/time.md）。
using clock = silicon::time::steady_clock;
using time_point = silicon::time::steady_clock::time_point;
}
