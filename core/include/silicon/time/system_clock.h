#pragma once

#include <chrono>
#include <cstdint>

namespace silicon::time {

class system_clock {
  public:
    std::chrono::system_clock::time_point now() const {
        return std::chrono::system_clock::now();
    }
    std::int64_t now_ms() const {
        using namespace std::chrono;
        return duration_cast<milliseconds>(now().time_since_epoch()).count();
    }
};

}
