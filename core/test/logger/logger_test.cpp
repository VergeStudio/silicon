#include <cstdint>
#include <source_location>
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

TEST_CASE("default_logger：init 成功后各级别日志函数可调用") {
    default_logger sink;
    auto r = sink.init("/tmp", log_level::kInfo, 8192, 1, 0);
    REQUIRE(r.has_value());

    sink.trace("trace msg", std::source_location::current());
    sink.debug("debug msg", std::source_location::current());
    sink.info("info msg", std::source_location::current());
    sink.warning("warn msg", std::source_location::current());
    sink.error("error msg", std::source_location::current());
    sink.critical("critical msg", std::source_location::current());
    CHECK(true);

    sink.stop();
}

TEST_CASE("default_logger：init 幂等，重复 init 仍返回 success") {
    default_logger sink;
    auto a = sink.init("/tmp", log_level::kInfo, 8192, 1, 0);
    auto b = sink.init("/tmp", log_level::kInfo, 8192, 1, 0);
    CHECK(a.has_value());
    CHECK(b.has_value());

    sink.stop();
}

TEST_CASE("default_logger：set_log_level 切换级别且不崩溃") {
    default_logger sink;
    auto r = sink.init("/tmp", log_level::kInfo, 8192, 1, 0);
    REQUIRE(r.has_value());

    sink.set_log_level(log_level::kDebug);
    sink.debug("debug after set_log_level", std::source_location::current());
    sink.set_log_level(log_level::kCritical);
    sink.critical("critical after set_log_level", std::source_location::current());
    CHECK(true);

    sink.stop();
}

TEST_CASE("make_logger_view：logger 作为依赖注入到消费方") {
    default_logger target;
    auto r = target.init("/tmp", log_level::kInfo, 8192, 1, 0);
    REQUIRE(r.has_value());

    logger_view view = make_logger_view(target);
    REQUIRE(view.has_value());
    view->info("injected info", std::source_location::current());
    view->set_log_level(log_level::kError);
    view->error("injected error", std::source_location::current());
    CHECK(true);

    target.stop();
}

TEST_CASE("logger error category 自注册：非注入消费方也能构造 error_code（不再 terminate）") {
    auto ec = make_error_code(logger_error::kInitFailed);
    CHECK(ec);
    CHECK(ec.value() == static_cast<int>(logger_error::kInitFailed));
    CHECK(std::string(ec.category().name()) == "silicon.logger");
    CHECK(ec.message() == "logger init failed");
    CHECK(make_error_code(logger_error::kInvalidLevel).message() == "invalid log level");
}
