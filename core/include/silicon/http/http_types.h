#pragma once

#include <map>
#include <memory>
#include <string>

namespace silicon::http {

/// http_response / http_request 为 header-only（global module 实体），原因与
/// silicon::time::system_clock 完全一致（见 time/system_clock.h 注释）：
/// 1. MSVC 对命名模块实体的修饰名追加模块标签（`::<!silicon.http>`），clang 不
///    生成该标签 → 任何「MSVC 编译的定义 + clang 引用」的跨工具链链接均无法解析；
/// 2. MSVC IFC 不含成员函数体，clang 导入模块后对成员函数发强引用，无法本地
///    内联展开。
/// 把这两个类型抽到纯文本头、在 silicon.http 模块接口的全局模块片段包含、并由
/// 消费方（如 siliconbuddy.client）直接 #include 本头，可让每个消费 TU 本地发射
/// （weak）符号，彻底规避跨 DLL / 跨工具链符号解析。
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

} // namespace silicon::http
