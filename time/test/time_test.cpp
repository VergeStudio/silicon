#include <cstdint>
#include <silicon/test/test.hpp>

import silicon.time;

using namespace silicon::time;

TEST_CASE("SystemClock 返回非零时间戳") {
    SystemClock clock;
    auto ms = clock.now_ms();
    CHECK(ms > 1'600'000'000'000LL);
}

TEST_CASE("DateSource 返回 YYYY-MM-DD 格式") {
    SystemClock clock;
    DateSource src(clock);
    auto date = src.current_date();
    CHECK(date.size() == 10);
    CHECK(date[4] == '-');
    CHECK(date[7] == '-');
}
