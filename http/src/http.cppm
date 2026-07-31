module;

#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

export module silicon.http;

export namespace silicon::http {

struct HttpResponse {

    struct P {
      public:
        int status_code = 0;
        std::string body;
        std::map<std::string, std::string> headers;
    };
    std::shared_ptr<P> m_p{std::make_shared<P>()};

  public:
    HttpResponse() = default;
    /// 便利构造：保留原聚合初始化 `HttpResponse{200, "{}"}` 的调用形态
    HttpResponse(int status, std::string b = {}, std::map<std::string, std::string> h = {}) {
        m_p->status_code = status;
        m_p->body = std::move(b);
        m_p->headers = std::move(h);
    }
    HttpResponse(const HttpResponse &o): m_p(std::make_shared<P>(*o.m_p)) {}
    HttpResponse &operator=(const HttpResponse &o) {
        if(this != &o) { m_p = std::make_shared<P>(*o.m_p); }
        return *this;
    }
    HttpResponse(HttpResponse &&) noexcept = default;
    HttpResponse &operator=(HttpResponse &&) noexcept = default;

  public:
    int &status_code() { return m_p->status_code; }
    const int &status_code() const { return m_p->status_code; }
    std::string &body() { return m_p->body; }
    const std::string &body() const { return m_p->body; }
    std::map<std::string, std::string> &headers() { return m_p->headers; }
    const std::map<std::string, std::string> &headers() const { return m_p->headers; }

};

struct HttpRequest {

    struct P {
      public:
        std::string url;
        std::string method = "GET";
        std::string body;
        std::map<std::string, std::string> headers;
        int timeout_ms = 30000;
    };
    std::shared_ptr<P> m_p{std::make_shared<P>()};

  public:
    HttpRequest() = default;
    HttpRequest(const HttpRequest &o): m_p(std::make_shared<P>(*o.m_p)) {}
    HttpRequest &operator=(const HttpRequest &o) {
        if(this != &o) { m_p = std::make_shared<P>(*o.m_p); }
        return *this;
    }
    HttpRequest(HttpRequest &&) noexcept = default;
    HttpRequest &operator=(HttpRequest &&) noexcept = default;

  public:
    std::string &url() { return m_p->url; }
    const std::string &url() const { return m_p->url; }
    std::string &method() { return m_p->method; }
    const std::string &method() const { return m_p->method; }
    std::string &body() { return m_p->body; }
    const std::string &body() const { return m_p->body; }
    std::map<std::string, std::string> &headers() { return m_p->headers; }
    const std::map<std::string, std::string> &headers() const { return m_p->headers; }
    int &timeout_ms() { return m_p->timeout_ms; }
    const int &timeout_ms() const { return m_p->timeout_ms; }

};

/// HTTP 客户端抽象（可注入，TDD 使用 FakeHttpClient）
class IHttpClient {
  public:
    virtual ~IHttpClient() = default;
    virtual HttpResponse request(const HttpRequest &req) const = 0;
    HttpResponse get(const std::string &url) const;
};

/// 基于 shell curl 的实现（沙箱内网络受限时可用本地模拟）
class CurlHttpClient: public IHttpClient {
  public:
    HttpResponse request(const HttpRequest &req) const override;
};

/// 打桩实现（返回预设响应，用于 TDD）
class FakeHttpClient: public IHttpClient {

    struct P {
      public:
        HttpResponse response_;
        mutable std::size_t call_count_ = 0;
    };
    std::unique_ptr<P> m_p{std::make_unique<P>()};

  public:
    explicit FakeHttpClient(HttpResponse response = {200, "{}"});
    HttpResponse request(const HttpRequest &) const override;
    std::size_t call_count() const;
};

} // namespace silicon::http
