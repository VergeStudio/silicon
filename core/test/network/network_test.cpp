#include <array>
#include <span>
#include <string>

#include <silicon/test/test.h>

import silicon.network;

using namespace silicon::network;

TEST_CASE("socket_address::create 解析合法 IPv4 并回环 ip/port/domain/to_string") {
    auto ep = socket_address::create("127.0.0.1", 8080);
    REQUIRE(ep.has_value());
    CHECK(ep->domain() == domain_t::kIpv4);
    CHECK(ep->port() == 8080);

    auto ip = ep->ip();
    REQUIRE(ip.has_value());
    CHECK(ip->domain() == domain_t::kIpv4);

    auto text = ep->to_string();
    REQUIRE(text.has_value());
    CHECK(*text == "127.0.0.1:8080");
}

TEST_CASE("socket_address::create 解析合法 IPv6 并回环") {
    auto ep = socket_address::create("::1", 9090, domain_t::kIpv6);
    REQUIRE(ep.has_value());
    CHECK(ep->domain() == domain_t::kIpv6);
    CHECK(ep->port() == 9090);

    auto text = ep->to_string();
    REQUIRE(text.has_value());
    CHECK(*text == "::1:9090");
}

TEST_CASE("socket_address::create 拒绝非法 IPv4 文本") {
    auto ep = socket_address::create("999.999.999.999", 80);
    CHECK_FALSE(ep.has_value());
    CHECK(ep.error() == make_error_code(network_error::kInvalidIpAddress));
}

TEST_CASE("socket_address 相等比较：同 ip+port 相等，端口不同则不等") {
    auto a = socket_address::create("10.0.0.1", 1234);
    auto b = socket_address::create("10.0.0.1", 1234);
    auto c = socket_address::create("10.0.0.1", 1235);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(c.has_value());
    CHECK(*a == *b);
    CHECK_FALSE(*a == *c);
}

TEST_CASE("ip_address::from_string/to_string 回环") {
    auto a = ip_address::from_string("192.168.1.1");
    REQUIRE(a.has_value());
    CHECK(a->domain() == domain_t::kIpv4);

    auto text = a->to_string();
    REQUIRE(text.has_value());
    CHECK(*text == "192.168.1.1");
}

TEST_CASE("ip_address::from_binary 拒绝超长数据") {
    std::array<uint8_t, 5> too_long{1, 2, 3, 4, 5};
    auto a = ip_address::from_binary(std::span<const uint8_t>{too_long.data(), too_long.size()},
                                     domain_t::kIpv4);
    CHECK_FALSE(a.has_value());
    CHECK(a.error() == make_error_code(network_error::kInvalidIpAddress));
}

TEST_CASE("to_string(connect_status) 返回有效字符串视图") {
    auto s = to_string(connect_status::kConnected);
    REQUIRE(s.has_value());
    CHECK_FALSE(s->empty());
}

TEST_CASE("hostname 值语义：深拷贝独立、按内容比较") {
    hostname h1{"example.com"};
    hostname h2 = h1;
    CHECK(h1 == h2);

    hostname h3{"other.org"};
    CHECK_FALSE(h1 == h3);
    CHECK(h1.data() == "example.com");
}
