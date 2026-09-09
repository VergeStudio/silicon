module;

#include <chrono>
#include <cstdint>

#include "silicon/common.h"
export module silicon.time.system_clock;

export namespace silicon::time {

class SILICON_CORE_API system_clock {
  public:
    // 时间点类型，与 std::chrono 时钟同构。
    using time_point = std::chrono::system_clock::time_point;

    std::chrono::system_clock::time_point now() const;
    std::int64_t now_ms() const;
};

}
