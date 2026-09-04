#pragma once

#include <map>
#include <memory>
#include <string>

namespace silicon::http {

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

}
