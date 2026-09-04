module;

#include <chrono>

export module silicon.scheduler:time;

export namespace silicon::scheduler {
using clock = std::chrono::steady_clock;
using time_point = clock::time_point;
}
