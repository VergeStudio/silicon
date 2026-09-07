module;

#include <map>
#include <memory>
#include <string>

#include "silicon/common.h"
export module silicon.http.types;

export namespace silicon::http {

struct CORE_API http_response {
    struct impl {
      public:
        int status_code_ = 0;
        std::string body_;
        std::map<std::string, std::string> headers_;
    };
    std::shared_ptr<impl> impl_{std::make_shared<impl>()};

  public:
    http_response() = default;
    http_response(int status, std::string b = {}, std::map<std::string, std::string> h = {});
    http_response(const http_response &o): impl_(std::make_shared<impl>(*o.impl_)) {}
    http_response &operator=(const http_response &o) {
        if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
        return *this;
    }
    http_response(http_response &&) noexcept = default;
    http_response &operator=(http_response &&) noexcept = default;

  public:
    int &status_code();
    const int &status_code() const;
    std::string &body();
    const std::string &body() const;
    std::map<std::string, std::string> &headers();
    const std::map<std::string, std::string> &headers() const;

};

struct CORE_API http_request {
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
    std::string &url();
    const std::string &url() const;
    std::string &method();
    const std::string &method() const;
    std::string &body();
    const std::string &body() const;
    std::map<std::string, std::string> &headers();
    const std::map<std::string, std::string> &headers() const;
    int &timeout_ms();
    const int &timeout_ms() const;

};

}
