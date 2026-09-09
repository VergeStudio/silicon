module;

#include <chrono>
#include <cstdint>

#include "silicon/common.h"
export module silicon.time.steady_clock;

export namespace silicon::time {

// 单调时钟：用于计时、超时与时间间隔度量。
//
// 语义与 std::chrono::steady_clock 完全一致——取值绝不回拨，不受系统时间
// 调整（NTP 校正、手动改时区等）影响，因此只适合度量"间隔"，不应作为墙上
// 时间戳使用（时间戳请用 system_clock）。
//
// 统一收敛到 core/time，避免各模块裸用 std::chrono::steady_clock，使时间来源
// 单一可替换（便于插桩、单测与跨平台一致性）。
//
// 接口刻意对齐 std::chrono 时钟：now()/now_ns()/now_ms() 均为静态成员，可像
// `std::chrono::steady_clock::now()` 那样以 `clock::now()` 形式调用，从而作为
// std::chrono 时钟的无缝替代品。
class SILICON_CORE_API steady_clock {
  public:
    // 时间点类型，与 std::chrono 时钟同构，便于作为 std::chrono 时钟的替代品。
    using time_point = std::chrono::steady_clock::time_point;

    // 当前单调时间点。
    [[nodiscard]] static std::chrono::steady_clock::time_point now();

    // 自单调纪元起的纳秒/毫秒整数计数，可用于日志时间戳、性能计数等。
    [[nodiscard]] static std::int64_t now_ns();
    [[nodiscard]] static std::int64_t now_ms();
};

}
