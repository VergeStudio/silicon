// config 模块测试：覆盖 config_value 标签联合值类型（谓词 / 访问器 / 值语义 / 类型不匹配抛
// 出）与 config error category 的专属错误码构造。
//
// 本用例刻意不调用 inject_config_error_category：config_error 的专属 category 现已由
// error.cppm 在模块静态初始化期自注册默认实例（与 silicon.network 一致），故未注入的消费方
// 也能安全走错误路径（不再 std::terminate）。若自注册失效，下面依赖错误返回路径的用例会崩溃。
#include <string>
#include <system_error>
#include <variant>

#include <silicon/test/test.h>

import silicon.config;

using namespace silicon::config;

// ---------- config_value：标签联合值类型 ----------
TEST_CASE("config_value 缺省构造为 null") {
    config_value v;
    CHECK(v.is_null());
    CHECK_FALSE(v.is_bool());
    CHECK_FALSE(v.is_int());
    CHECK_FALSE(v.is_double());
    CHECK_FALSE(v.is_string());
}

TEST_CASE("config_value 各类型构造与 is_* 谓词") {
    config_value b{true};
    CHECK(b.is_bool());
    config_value i{static_cast<int64_t>(42)};
    CHECK(i.is_int());
    config_value d{3.14};
    CHECK(d.is_double());
    config_value s{std::string{"hello"}};
    CHECK(s.is_string());
}

TEST_CASE("config_value as_* 返回所存值") {
    CHECK(config_value{true}.as_bool() == true);
    CHECK(config_value{static_cast<int64_t>(-7)}.as_int() == -7);
    CHECK(config_value{2.5}.as_double() == 2.5);
    CHECK(config_value{std::string{"x"}}.as_string() == "x");
}

TEST_CASE("config_value as_string_opt：string 返回指针、其它类型返回 nullptr") {
    config_value s{std::string{"opt"}};
    const std::string *p = s.as_string_opt();
    REQUIRE(p != nullptr);
    CHECK(*p == "opt");

    config_value i{static_cast<int64_t>(1)};
    CHECK(i.as_string_opt() == nullptr);
}

TEST_CASE("config_value 拷贝赋值深拷贝：改原值不影响副本") {
    config_value a{std::string{"orig"}};
    config_value b = a;                          // 深拷贝
    a = config_value{static_cast<int64_t>(99)}; // 重赋值 a

    CHECK(b.is_string());
    CHECK(b.as_string() == "orig"); // 副本独立
    CHECK(a.is_int());
    CHECK(a.as_int() == 99);
}

TEST_CASE("config_value 移动构造转移值") {
    config_value a{std::string{"moved"}};
    config_value c{std::move(a)}; // 移动后不再访问 a（其 impl_ 可能为空）
    CHECK(c.is_string());
    CHECK(c.as_string() == "moved");
}

TEST_CASE("config_value as_* 类型不匹配抛 std::bad_variant_access") {
    config_value s{std::string{"not_a_bool"}};
    // as_* 为 [[nodiscard]]，CHECK_THROWS_AS 内以丢弃值表达式求值，需显式
    // static_cast<void> 丢弃返回值；求值语义不变，异常仍照常传播。
    CHECK_THROWS_AS(static_cast<void>(s.as_bool()), std::bad_variant_access);
    CHECK_THROWS_AS(static_cast<void>(s.as_int()), std::bad_variant_access);
    CHECK_THROWS_AS(static_cast<void>(s.as_double()), std::bad_variant_access);

    config_value i{static_cast<int64_t>(1)};
    CHECK_THROWS_AS(static_cast<void>(i.as_string()), std::bad_variant_access);
}

// ---------- config error category（自注册消费方验证） ----------
TEST_CASE("config error category 自注册：非注入消费方也能构造 error_code（不再 terminate）") {
    auto ec = make_error_code(config_error::kLoadFailed);
    CHECK(ec); // 已置错误
    CHECK(ec.value() == static_cast<int>(config_error::kLoadFailed));
    CHECK(std::string(ec.category().name()) == "silicon.config");
    CHECK(ec.message() == "config load failed");

    CHECK(make_error_code(config_error::kParseFailed).message() == "config parse failed");
    CHECK(make_error_code(config_error::kInvalidValue).message() == "invalid config value");
    CHECK(make_error_code(config_error::kUnknown).message() == "unknown config error");
}
