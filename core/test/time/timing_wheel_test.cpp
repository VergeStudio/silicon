#include <cstddef>
#include <chrono>
#include <vector>

#include <silicon/test/test.h>

import silicon.time;

using namespace silicon::time;
using namespace std::chrono_literals;

// 便利工厂：就地构造返回，依赖 C++17 强制 copy elision（timing_wheel 禁拷贝/移动）。
// 注意：仅可在声明处直接作为初始化，禁止对既有对象赋值/移动。
static timing_wheel make_wheel(std::size_t slots = 64) {
    return timing_wheel{timing_wheel::options{1ms, slots}};
}

TEST_CASE("timing_wheel: 一次性定时，推进 delay 后触发恰一次") {
    auto w = make_wheel();
    int fired = 0;
    auto h = w.add_once(5ms, [&] { ++fired; });
    CHECK(h.valid());
    CHECK(w.pending() == 1);

    for(int i = 0; i < 4; ++i) {
        CHECK(w.advance() == 0);
        CHECK(fired == 0);
    }
    // 第 5 次推进到期。
    CHECK(w.advance() == 1);
    CHECK(fired == 1);
    // 过期后不再触发。
    CHECK(w.advance() == 0);
    CHECK(fired == 1);
    CHECK(w.pending() == 0);
}

TEST_CASE("timing_wheel: 周期定时在 t/2t/3t 各触发一次") {
    auto w = make_wheel();
    int fired = 0;
    auto h = w.add_periodic(3ms, [&] { ++fired; });
    CHECK(w.pending() == 1);

    w.advance(); // t=1
    w.advance(); // t=2
    CHECK(fired == 0);
    w.advance(); // t=3 首次
    CHECK(fired == 1);
    w.advance();
    w.advance(); // t=5
    CHECK(fired == 1);
    w.advance(); // t=6 二次
    CHECK(fired == 2);
    w.advance();
    w.advance();
    w.advance(); // t=9 三次
    CHECK(fired == 3);
    CHECK(w.pending() == 1); // 周期项仍活跃
}

TEST_CASE("timing_wheel: cancel 后不再触发") {
    auto w = make_wheel();
    int fired = 0;
    auto h = w.add_periodic(2ms, [&] { ++fired; });
    w.advance();
    w.advance(); // 首次
    CHECK(fired == 1);
    w.cancel(h);
    CHECK(w.pending() == 0);
    for(int i = 0; i < 10; ++i) {
        w.advance();
    }
    CHECK(fired == 1); // 不再增长
}

TEST_CASE("timing_wheel: 同槽多个到期项按注册顺序回调") {
    auto w = make_wheel();
    std::vector<int> order;
    w.add_once(2ms, [&] { order.push_back(1); });
    w.add_once(2ms, [&] { order.push_back(2); });
    w.add_once(3ms, [&] { order.push_back(3); });

    w.advance();
    w.advance(); // t=2 槽：1、2
    CHECK(order == std::vector<int>({1, 2}));
    w.advance(); // t=3 槽：3
    CHECK(order == std::vector<int>({1, 2, 3}));
}

TEST_CASE("timing_wheel: 跨多槽不同 delay 均不漏触发") {
    auto w = make_wheel();
    int a = 0, b = 0, c = 0;
    w.add_once(1ms, [&] { ++a; });
    w.add_once(7ms, [&] { ++b; });
    w.add_once(32ms, [&] { ++c; });

    // 推进 32 格，三个都触发一次，且各时段正确。
    for(int i = 1; i <= 32; ++i) {
        w.advance();
    }
    CHECK(a == 1);
    CHECK(b == 1);
    CHECK(c == 1);
}

TEST_CASE("timing_wheel: 超窗口跨度的延时走溢出列表兜底到点触发") {
    // 8 槽、1ms → 窗口 7ms；delay 远超窗口走 overrun。
    auto w = make_wheel(8);
    int fired = 0;
    w.add_once(50ms, [&] { ++fired; });
    CHECK(w.pending() == 1);

    for(int i = 0; i < 49; ++i) {
        CHECK(w.advance() == 0);
    }
    CHECK(fired == 0);
    CHECK(w.advance() == 1); // t=50 触发
    CHECK(fired == 1);
    CHECK(w.pending() == 0);
}

TEST_CASE("timing_wheel: advance_by 按整除 tick 推进，足额才触发") {
    // slot_duration=2ms → advance_by 以 2ms 为格。
    auto w = timing_wheel{timing_wheel::options{2ms, 64}};
    int fired = 0;
    w.add_once(6ms, [&] { ++fired; }); // 需 3 格

    CHECK(w.advance_by(5ms) == 0); // floor(5/2)=2 格，不足
    CHECK(fired == 0);
    CHECK(w.advance_by(1ms) == 0); // floor(1/2)=0 格，余数丢弃
    CHECK(fired == 0);
    CHECK(w.advance_by(2ms) == 1); // 累计 3 格触发
    CHECK(fired == 1);
}

TEST_CASE("timing_wheel: 回调内再次 add 与 cancel 自身稳定") {
    auto w = make_wheel();
    int one = 0;
    int inner = 0;
    timer_handle h; // 延迟赋值，避免 lambda 捕获未初始化句柄

    h = w.add_periodic(2ms, [&] {
        ++one;
        if(one == 1) {
            w.add_once(2ms, [&] { ++inner; }); // 回调内新增
            w.cancel(h);                       // 回调内取消自身
        }
    });

    w.advance();
    w.advance(); // t=2 首次触发：one=1，cancel 自身，注册 inner
    CHECK(one == 1);
    w.advance();
    w.advance(); // t=4 inner 触发
    CHECK(inner == 1);
    // 自身已取消，后续不再触发
    for(int i = 0; i < 10; ++i) {
        w.advance();
    }
    CHECK(one == 1);
}
