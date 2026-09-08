#include <cstdint>
#include <chrono>
#include <silicon/test/test.h>

import silicon.time;

using namespace silicon::time;

TEST_CASE("system_clock 返回非零时间戳") {
    system_clock clock;
    auto ms = clock.now_ms();
    CHECK(ms > 1'600'000'000'000LL);
}

TEST_CASE("i_date_source 返回 YYYY-MM-DD 格式") {
    system_clock clock;
    date_source src(make_clock<system_clock>());
    auto date = src.current_date();
    CHECK(date.size() == 10);
    CHECK(date[4] == '-');
    CHECK(date[7] == '-');
}

TEST_CASE("steady_clock 返回单调不减的时间点") {
    steady_clock clk;
    auto t0 = clk.now();
    auto t1 = clk.now();
    CHECK(t1 >= t0);
    // 两次相邻取样间隔不应为负（单调时钟保证）。
    auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    CHECK(delta >= 0);
}

TEST_CASE("steady_clock now_ms 为合理毫秒计数") {
    steady_clock clk;
    auto ms = clk.now_ms();
    // 单调纪元毫秒计数应为正且为 64 位量级。
    CHECK(ms > 0);
}

TEST_CASE("sleep_for 实际休眠约指定时长") {
    steady_clock clk;
    auto t0 = clk.now();
    sleep_for(std::chrono::milliseconds{10});
    auto t1 = clk.now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    // 允许少量调度抖动，休眠应至少接近 10ms。
    CHECK(elapsed >= 9);
}
