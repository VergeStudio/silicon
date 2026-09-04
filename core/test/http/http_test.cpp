#include <map>
#include <string>

#include <silicon/test/test.h>
#include <silicon/http/http_types.h>

using namespace silicon::http;

TEST_CASE("http_response 便利构造：status/body/headers 按参写入") {
    std::map<std::string, std::string> h{{"Content-Type", "application/json"}};
    http_response r{200, "{}", h};

    CHECK(r.status_code() == 200);
    CHECK(r.body() == "{}");
    CHECK(r.headers() == h);
}

TEST_CASE("http_response 缺省构造：零状态、空负载") {
    http_response r;
    CHECK(r.status_code() == 0);
    CHECK(r.body().empty());
    CHECK(r.headers().empty());
}

TEST_CASE("http_response 深拷贝独立：改副本不影响原值") {
    http_response a{201, "orig", {{"X", "1"}}};
    http_response b = a;

    b.status_code() = 404;
    b.body() = "mutated";
    b.headers()["X"] = "2";

    CHECK(a.status_code() == 201);
    CHECK(a.body() == "orig");
    CHECK(a.headers()["X"] == "1");
    CHECK(b.status_code() == 404);
    CHECK(b.body() == "mutated");
    CHECK(b.headers()["X"] == "2");
}

TEST_CASE("http_response setter 经引用修改原 pimpl") {
    http_response r{500, "err"};
    r.status_code() = 503;
    r.body() = "unavailable";
    r.headers()["Retry-After"] = "30";

    CHECK(r.status_code() == 503);
    CHECK(r.body() == "unavailable");
    CHECK(r.headers().at("Retry-After") == "30");
}

TEST_CASE("http_request 缺省构造：method=GET、timeout=30000") {
    http_request req;
    CHECK(req.method() == "GET");
    CHECK(req.timeout_ms() == 30000);
    CHECK(req.url().empty());
    CHECK(req.body().empty());
    CHECK(req.headers().empty());
}

TEST_CASE("http_request setter 修改 url/method/body/headers/timeout") {
    http_request req;
    req.url() = "https://api.example.com/v1";
    req.method() = "POST";
    req.body() = "payload";
    req.headers()["Authorization"] = "Bearer x";
    req.timeout_ms() = 5000;

    CHECK(req.url() == "https://api.example.com/v1");
    CHECK(req.method() == "POST");
    CHECK(req.body() == "payload");
    CHECK(req.headers().at("Authorization") == "Bearer x");
    CHECK(req.timeout_ms() == 5000);
}

TEST_CASE("http_request 深拷贝独立：改副本不影响原值") {
    http_request a;
    a.url() = "https://a.test";
    a.method() = "PUT";
    a.timeout_ms() = 1000;

    http_request b = a;
    b.url() = "https://b.test";
    b.method() = "DELETE";
    b.timeout_ms() = 2000;

    CHECK(a.url() == "https://a.test");
    CHECK(a.method() == "PUT");
    CHECK(a.timeout_ms() == 1000);
    CHECK(b.url() == "https://b.test");
    CHECK(b.method() == "DELETE");
    CHECK(b.timeout_ms() == 2000);
}
