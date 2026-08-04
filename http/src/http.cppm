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

    struct Impl {
      public:
        int status_code_ = 0;
        std::string body_;
        std::map<std::string, std::string> headers_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    HttpResponse() = default;
    /// 便利构造：保留原聚合初始化 `HttpResponse{200, "{}"}` 的调用形态
    HttpResponse(int status, std::string b = {}, std::map<std::string, std::string> h = {}) {
        impl_->status_code_ = status;
        impl_->body_ = std::move(b);
        impl_->headers_ = std::move(h);
    }
    HttpResponse(const HttpResponse &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    HttpResponse &operator=(const HttpResponse &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    HttpResponse(HttpResponse &&) noexcept = default;
    HttpResponse &operator=(HttpResponse &&) noexcept = default;

  public:
    int &StatusCode() { return impl_->status_code_; }
    const int &StatusCode() const { return impl_->status_code_; }
    std::string &Body() { return impl_->body_; }
    const std::string &Body() const { return impl_->body_; }
    std::map<std::string, std::string> &Headers() { return impl_->headers_; }
    const std::map<std::string, std::string> &Headers() const { return impl_->headers_; }

};

struct HttpRequest {

    struct Impl {
      public:
        std::string url_;
        std::string method_ = "GET";
        std::string body_;
        std::map<std::string, std::string> headers_;
        int timeout_ms_ = 30000;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    HttpRequest() = default;
    HttpRequest(const HttpRequest &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    HttpRequest &operator=(const HttpRequest &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    HttpRequest(HttpRequest &&) noexcept = default;
    HttpRequest &operator=(HttpRequest &&) noexcept = default;

  public:
    std::string &Url() { return impl_->url_; }
    const std::string &Url() const { return impl_->url_; }
    std::string &Method() { return impl_->method_; }
    const std::string &Method() const { return impl_->method_; }
    std::string &Body() { return impl_->body_; }
    const std::string &Body() const { return impl_->body_; }
    std::map<std::string, std::string> &Headers() { return impl_->headers_; }
    const std::map<std::string, std::string> &Headers() const { return impl_->headers_; }
    int &TimeoutMs() { return impl_->timeout_ms_; }
    const int &TimeoutMs() const { return impl_->timeout_ms_; }

};

/// HTTP 客户端抽象（可注入，TDD 使用 FakeHttpClient）
class IHttpClient {
  public:
    virtual ~IHttpClient() = default;
    virtual HttpResponse request(const HttpRequest &req) const = 0;
    HttpResponse Get(const std::string &url) const;
};

/// 基于 shell curl 的实现（沙箱内网络受限时可用本地模拟）
class CurlHttpClient: public IHttpClient {
  public:
    HttpResponse request(const HttpRequest &req) const override;
};

/// 打桩实现（返回预设响应，用于 TDD）
class FakeHttpClient: public IHttpClient {

    struct Impl {
      public:
        HttpResponse response_;
        mutable std::size_t call_count_ = 0;
    };
    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};

  public:
    explicit FakeHttpClient(HttpResponse response = {200, "{}"});
    HttpResponse request(const HttpRequest &) const override;
    std::size_t CallCount() const;
};

} // namespace silicon::http
