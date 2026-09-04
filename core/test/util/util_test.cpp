#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include <silicon/test/test.h>

import silicon.util;

using namespace silicon::util;
using namespace silicon::os;

TEST_CASE("has_suffix 命中与未命中") {
    CHECK(has_suffix("filename.txt", ".txt"));
    CHECK_FALSE(has_suffix("filename.txt", ".png"));
    CHECK(has_suffix("archive.tar.gz", ".gz"));
    CHECK_FALSE(has_suffix("archive.tar.gz", ".tar"));
    CHECK_FALSE(has_suffix("short", "longer_than_str"));
    CHECK(has_suffix("anything", ""));
    CHECK_FALSE(has_suffix("FILE.TXT", ".txt"));
}

TEST_CASE("generate_unique_id 单调递增且唯一") {
    std::uint64_t a = generate_unique_id();
    std::uint64_t b = generate_unique_id();
    std::uint64_t c = generate_unique_id();
    CHECK(a < b);
    CHECK(b < c);
    CHECK(a != b);
    CHECK(b != c);
}

TEST_CASE("str_cat 拼接 0~N 个 string-like 参数") {
    CHECK(str_cat() == "");
    CHECK(str_cat("a") == "a");
    CHECK(str_cat("a", "b", "c") == "abc");
    CHECK(str_cat("x", std::string("y"), std::string_view("z")) == "xyz");
    CHECK(str_cat("num=", "42") == "num=42");
}

TEST_CASE("str_append 就地追加到已有字符串") {
    std::string s;
    str_append(&s, "a", "b");
    CHECK(s == "ab");

    std::string t{"pre"};
    str_append(&t, "fix", std::string("-post"));
    CHECK(t == "prefix-post");
}

TEST_CASE("os::get_env 读取已设置变量与未设置变量") {
    set_env("SILICON_UTIL_TEST_VAR", "hello");
    CHECK(get_env("SILICON_UTIL_TEST_VAR") == "hello");

    CHECK(get_env("SILICON_UTIL_TEST_VAR_DEFINITELY_UNSET") == "");
}
