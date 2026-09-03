// error 模块测试：锁定统一错误返回类型契约。
//
// silicon.error 极薄：仅导出 `result<T>` 别名（= std::expected<T, std::error_code>）与
// `category_deleter`（承载「模块独占 category 句柄」的 unique_ptr 所用 no-op 删除器，
// 因 std::error_category 析构为保护、category 进程期常驻）。测试聚焦于这两者的契约，
// 确保后续各模块经 `import silicon.error;` 复用的形态稳定。
#include <concepts>
#include <expected>
#include <memory>
#include <system_error>
#include <type_traits>

#include <silicon/test/test.h>

import silicon.error;

using namespace silicon::error;

TEST_CASE("result 别名契约：result<T> 严格等价于 std::expected<T, std::error_code>") {
    static_assert(std::is_same_v<result<int>, std::expected<int, std::error_code>>);
    static_assert(std::is_same_v<result<void>, std::expected<void, std::error_code>>);
    static_assert(std::is_same_v<result<std::string>, std::expected<std::string, std::error_code>>);
}

TEST_CASE("result 成功路径：has_value / value / operator*") {
    result<int> ok{42};
    CHECK(ok.has_value());
    CHECK(ok.value() == 42);
    CHECK(*ok == 42);

    result<std::string> s{std::in_place, "hello"};
    REQUIRE(s.has_value());
    CHECK(*s == "hello");
}

TEST_CASE("result<void> 成功路径") {
    result<void> ok{std::in_place};
    CHECK(ok.has_value());
}

TEST_CASE("result 失败路径：error_code 可用、!has_value") {
    std::error_code ec = std::make_error_code(std::errc::no_such_file_or_directory);
    result<int> fail{std::unexpected(ec)};
    CHECK_FALSE(fail.has_value());
    CHECK(fail.error() == ec);
    CHECK(fail.error() == std::make_error_code(std::errc::no_such_file_or_directory));
}

TEST_CASE("category_deleter 契约：可调用、noexcept、包真实 category 不释放") {
    static_assert(std::is_invocable_v<category_deleter, const std::error_category*>);
    static_assert(noexcept(std::declval<category_deleter>()(std::declval<const std::error_category*>())));

    // 包真实 category，reset 应安全（无 delete、不崩溃）
    std::unique_ptr<const std::error_category, category_deleter> hold{&std::system_category()};
    REQUIRE(hold.get() == &std::system_category());
    hold.reset();
    CHECK(true);
}

TEST_CASE("category_deleter 对空指针调用为 no-op") {
    category_deleter d;
    d(nullptr); // 不得解引用/崩溃
    CHECK(true);
}
