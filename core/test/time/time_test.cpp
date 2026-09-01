#include <cstdint>
#include <silicon/test/test.hpp>
#include <silicon/time/system_clock.h>

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
