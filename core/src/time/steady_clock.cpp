module;

#include <chrono>
#include <cstdint>

module silicon.time.steady_clock;

namespace silicon::time {

std::chrono::steady_clock::time_point steady_clock::now() {
    return std::chrono::steady_clock::now();
}

std::int64_t steady_clock::now_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(now().time_since_epoch()).count();
}

std::int64_t steady_clock::now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(now().time_since_epoch()).count();
}

}
