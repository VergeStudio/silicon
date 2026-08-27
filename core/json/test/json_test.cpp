#include <silicon/test/test.hpp>
#include <string>
#include <string_view>

import std;
import silicon.json;

using json = silicon::json::json;

TEST_CASE("parse: 基础标量") {
    auto r = json::parse(R"(42)", nullptr, false);
    REQUIRE(!r.is_discarded());
    CHECK(r.is_number_integer());
    CHECK(r.get<int>() == 42);

    auto b = json::parse(R"(true)", nullptr, false);
    REQUIRE(!b.is_discarded());
    CHECK(b.is_boolean());
    CHECK(b.get<bool>());

    auto n = json::parse(R"(null)", nullptr, false);
    REQUIRE(!n.is_discarded());
    CHECK(n.is_null());

    auto d = json::parse(R"(3.14)", nullptr, false);
    REQUIRE(!d.is_discarded());
    CHECK(d.is_number_float());
    CHECK(d.get<double>() == doctest::Approx(3.14));
}

TEST_CASE("parse: 字符串与转义") {
    auto r = json::parse(R"("hello \"world\"\n")", nullptr, false);
    REQUIRE(!r.is_discarded());
    CHECK(r.is_string());
    CHECK(r.get<std::string>() == std::string("hello \"world\"\n"));
}

TEST_CASE("parse: 数组与对象") {
    auto r = json::parse(R"({"name":"siliconcode","tags":["a","b"],"n":7})", nullptr, false);
    REQUIRE(!r.is_discarded());
    CHECK(r.is_object());
    CHECK(r["name"].get<std::string>() == "siliconcode");
    REQUIRE(r["tags"].is_array());
    CHECK(r["tags"].size() == 2);
    CHECK(r["n"].get<int>() == 7);
}

TEST_CASE("parse: 非法输入抛出异常") {
    json r_invalid;
    CHECK_THROWS_AS(r_invalid = json::parse(R"({invalid})"), json::parse_error);
}

TEST_CASE("dump 往返一致性") {
    std::string_view src = R"({"x":1,"y":[true,null,"z"],"w":2.5})";
    auto v = json::parse(src, nullptr, false);
    REQUIRE(!v.is_discarded());
    auto text = v.dump();
    auto back = json::parse(text, nullptr, false);
    REQUIRE(!back.is_discarded());
    CHECK(back.dump() == text);
}

TEST_CASE("dump: 有序输出") {
    json v;
    v["b"] = 1;
    v["a"] = 2;
    CHECK(v.dump() == std::string(R"({"a":2,"b":1})"));
}
