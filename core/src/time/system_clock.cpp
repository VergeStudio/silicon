module;

#include <chrono>
#include <cstdint>

module silicon.time.system_clock;

namespace silicon::time {

std::chrono::system_clock::time_point system_clock::now() const {
    return std::chrono::system_clock::now();
}

std::int64_t system_clock::now_ms() const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(now().time_since_epoch()).count();
}

}
