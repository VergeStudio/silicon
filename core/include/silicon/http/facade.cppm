module;

#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>

#include <tuple>
// proxy dispatch 宏头：宏不随 C++20 模块导出，必须在全局模块片段文本包含
#include <silicon/proxy/proxy_macros.h>
#include <silicon/common.h>
export module silicon.http;

export import silicon.http.error;


import silicon.proxy;

export namespace silicon::http {

struct http_response {

    struct impl {
      public:
        int status_code_ = 0;
        std::string body_;
        std::map<std::string, std::string> headers_;
    };
    std::shared_ptr<impl> impl_{std::make_shared<impl>()};

  public:
    http_response() = default;
    /// 便利构造：保留原聚合初始化 `http_response{200, "{}"}` 的调用形态
    http_response(int status, std::string b = {}, std::map<std::string, std::string> h = {}) {
        impl_->status_code_ = status;
        impl_->body_ = std::move(b);
        impl_->headers_ = std::move(h);
    }
    http_response(const http_response &o): impl_(std::make_shared<impl>(*o.impl_)) {}
    http_response &operator=(const http_response &o) {
        if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
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

    struct impl {
      public:
        std::string url_;
        std::string method_ = "GET";
        std::string body_;
        std::map<std::string, std::string> headers_;
        int timeout_ms_ = 30000;
    };
    std::shared_ptr<impl> impl_{std::make_shared<impl>()};

  public:
    http_request() = default;
    http_request(const http_request &o): impl_(std::make_shared<impl>(*o.impl_)) {}
    http_request &operator=(const http_request &o) {
        if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
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

/// HTTP 客户端门面（type-erased，鸭子类型满足即可）
PRO_DEF_MEM_DISPATCH(MemHttpClientRequest, request);
struct http_client_facade : silicon::proxy::facade_builder
    ::add_convention<MemHttpClientRequest,
                     http_response(const http_request &) const>::build {};

using http_client_proxy = silicon::proxy::proxy<http_client_facade>;
using http_client_view = silicon::proxy::proxy_view<http_client_facade>;

template <class T, class... Args>
[[nodiscard]] http_client_proxy make_http_client(Args &&...args) {
    return silicon::proxy::make_proxy<http_client_facade, T>(
        std::forward<Args>(args)...);
}

/// 便利封装：等价于对 url 发起一次 GET request
inline http_response get(const http_client_view &client, const std::string &url) {
    http_request req;
    req.url() = url;
    return client->request(req);
}

/// 基于 shell curl 的实现（沙箱内网络受限时可用本地模拟）
class CORE_API curl_http_client {
  public:
    http_response request(const http_request &) const;
};

/// 打桩实现（返回预设响应，用于 TDD）
class CORE_API fake_http_client {

    struct impl {
      public:
        http_response response_;
        mutable std::size_t call_count_ = 0;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:
    explicit fake_http_client(http_response = {200, "{}"});
    http_response request(const http_request &) const;
    std::size_t call_count() const;
};

} // namespace silicon::http
