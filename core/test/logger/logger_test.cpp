






#include <cstdint>
#include <string>

#include <silicon/test/test.h>

import silicon.logger;

using namespace silicon::logger;

TEST_CASE("log_level 枚举底层值严格递增（kTrace..kOff）") {
    static_assert(static_cast<std::uint8_t>(log_level::kTrace) == 0);
    static_assert(static_cast<std::uint8_t>(log_level::kDebug) == 1);
    static_assert(static_cast<std::uint8_t>(log_level::kInfo) == 2);
    static_assert(static_cast<std::uint8_t>(log_level::kWarn) == 3);
    static_assert(static_cast<std::uint8_t>(log_level::kError) == 4);
    static_assert(static_cast<std::uint8_t>(log_level::kCritical) == 5);
    static_assert(static_cast<std::uint8_t>(log_level::kOff) == 6);
}

TEST_CASE("init 成功后返回 expected success，且各级别日志函数可调用") {
    auto r = init("/tmp", log_level::kInfo, 8192, 1, 0);
    REQUIRE(r.has_value());

    trace("trace msg");
    debug("debug msg");
    info("info msg");
    warning("warn msg");
    error("error msg");
    critical("critical msg");
    CHECK(true);

    stop();
}

TEST_CASE("init 幂等：重复 init 仍返回 success") {
    auto a = init("/tmp", log_level::kInfo, 8192, 1, 0);
    auto b = init("/tmp", log_level::kInfo, 8192, 1, 0);
    CHECK(a.has_value());
    CHECK(b.has_value());

    stop();
}

TEST_CASE("set_log_level 切换级别且不崩溃") {
    auto r = init("/tmp", log_level::kInfo, 8192, 1, 0);
    REQUIRE(r.has_value());

    set_log_level(log_level::kDebug);
    debug("debug after set_log_level");
    set_log_level(log_level::kCritical);
    critical("critical after set_log_level");
    CHECK(true);

    stop();
}

TEST_CASE("logger error category 自注册：非注入消费方也能构造 error_code（不再 terminate）") {
    auto ec = make_error_code(logger_error::kInitFailed);
    CHECK(ec);
    CHECK(ec.value() == static_cast<int>(logger_error::kInitFailed));
    CHECK(std::string(ec.category().name()) == "silicon.logger");
    CHECK(ec.message() == "logger init failed");
    CHECK(make_error_code(logger_error::kInvalidLevel).message() == "invalid log level");
}
