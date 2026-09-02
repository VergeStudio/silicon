// library 模块测试：覆盖 flags 枚举契约、prefix/suffix/get_os_name 平台名构造、shared_library
// 缺省构造状态，以及 load/get_symbol 失败路径（均经 make_error_code 触发 library category）。
//
// library_error 的专属 category 现已由 error.cppm 在模块静态初始化期自注册默认实例（与
// silicon.network / silicon.config / silicon.logger / silicon.event 一致），故未注入的消费方
// 走错误路径（make_error_code）前 category 已可用，不再 std::terminate。
#include <string>

#include <silicon/test/test.hpp>

import silicon.library;

using namespace silicon::library;

TEST_CASE("flags 枚举底层值（kShLibGlobal/kShLibLocal）") {
    static_assert(static_cast<int>(shared_library::flags::kShLibGlobal) == 1);
    static_assert(static_cast<int>(shared_library::flags::kShLibLocal) == 2);
}

TEST_CASE("prefix/suffix 非空且 get_os_name 由其拼接") {
    CHECK_FALSE(shared_library::prefix().empty());
    CHECK_FALSE(shared_library::suffix().empty());
    CHECK(shared_library::get_os_name("foo") == shared_library::prefix() + "foo" + shared_library::suffix());
}

TEST_CASE("shared_library 缺省构造：未加载、路径为空") {
    shared_library lib;
    CHECK_FALSE(lib.is_loaded());
    CHECK(lib.get_path().empty());
}

TEST_CASE("shared_library load 不存在的路径返回 library_error::kLoadFailed") {
    shared_library lib;
    auto r = lib.load("/this/path/should/not/exist/libnope.dylib");
    CHECK_FALSE(r.has_value());
    CHECK(r.error() == make_error_code(library_error::kLoadFailed));
    CHECK(std::string(r.error().category().name()) == "silicon.library");
}

TEST_CASE("shared_library get_symbol 未加载库返回 kSymbolNotFound") {
    shared_library lib;
    auto s = lib.get_symbol("some_symbol");
    CHECK_FALSE(s.has_value());
    CHECK(s.error() == make_error_code(library_error::kSymbolNotFound));
}

TEST_CASE("library error category 自注册：非注入消费方也能构造 error_code（不再 terminate）") {
    auto ec = make_error_code(library_error::kInvalidHandle);
    CHECK(ec); // 已置错误
    CHECK(ec.value() == static_cast<int>(library_error::kInvalidHandle));
    CHECK(std::string(ec.category().name()) == "silicon.library");
    CHECK(ec.message() == "invalid shared library handle");
}
