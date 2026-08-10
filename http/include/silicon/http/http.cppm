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

struct http_response {

    struct Impl {
      public:
        int status_code_ = 0;
        std::string body_;
        std::map<std::string, std::string> headers_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    http_response() = default;
    /// 便利构造：保留原聚合初始化 `http_response{200, "{}"}` 的调用形态
    http_response(int status, std::string b = {}, std::map<std::string, std::string> h = {}) {
        impl_->status_code_ = status;
        impl_->body_ = std::move(b);
        impl_->headers_ = std::move(h);
    }
    http_response(const http_response &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    http_response &operator=(const http_response &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    http_response(http_response &&) noexcept = default;
    http_response &operator=(http_response &&) noexcept = default;

  public:
    int &status_code() { return impl_->status_code_; }
    const int &status_code() const { return impl_->status_code_; }
    std::string &body() { return impl_->body_; }
    const std::string &body() const { return impl_->body_; }
    std::map<std::string, std::string> &headers() { return impl_->headers_; }
    const std::map<std::string, std::string> &headers() const { return impl_->headers_; }

};

struct http_request {

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
    http_request() = default;
    http_request(const http_request &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    http_request &operator=(const http_request &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    http_request(http_request &&) noexcept = default;
    http_request &operator=(http_request &&) noexcept = default;

  public:
    std::string &url() { return impl_->url_; }
    const std::string &url() const { return impl_->url_; }
    std::string &method() { return impl_->method_; }
    const std::string &method() const { return impl_->method_; }
    std::string &body() { return impl_->body_; }
    const std::string &body() const { return impl_->body_; }
    std::map<std::string, std::string> &headers() { return impl_->headers_; }
    const std::map<std::string, std::string> &headers() const { return impl_->headers_; }
    int &timeout_ms() { return impl_->timeout_ms_; }
    const int &timeout_ms() const { return impl_->timeout_ms_; }

};

/// HTTP 客户端抽象（可注入，TDD 使用 fake_http_client）
class i_http_client {
  public:
    virtual ~i_http_client() = default;
    virtual http_response request(const http_request &req) const = 0;
    http_response get(const std::string &url) const;
};

/// 基于 shell curl 的实现（沙箱内网络受限时可用本地模拟）
class curl_http_client: public i_http_client {
  public:
    http_response request(const http_request &req) const override;
};

/// 打桩实现（返回预设响应，用于 TDD）
class fake_http_client: public i_http_client {

    struct Impl {
      public:
        http_response response_;
        mutable std::size_t call_count_ = 0;
    };
    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};

  public:
    explicit fake_http_client(http_response response = {200, "{}"});
    http_response request(const http_request &) const override;
    std::size_t call_count() const;
};

} // namespace silicon::http
