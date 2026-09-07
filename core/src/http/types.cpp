module;

#include <map>
#include <string>
#include <utility>

module silicon.http.types;

namespace silicon::http {

http_response::http_response(int status, std::string b, std::map<std::string, std::string> h) {
    impl_->status_code_ = status;
    impl_->body_ = std::move(b);
    impl_->headers_ = std::move(h);
}

int &http_response::status_code() { return impl_->status_code_; }
const int &http_response::status_code() const { return impl_->status_code_; }
std::string &http_response::body() { return impl_->body_; }
const std::string &http_response::body() const { return impl_->body_; }
std::map<std::string, std::string> &http_response::headers() { return impl_->headers_; }
const std::map<std::string, std::string> &http_response::headers() const { return impl_->headers_; }

std::string &http_request::url() { return impl_->url_; }
const std::string &http_request::url() const { return impl_->url_; }
std::string &http_request::method() { return impl_->method_; }
const std::string &http_request::method() const { return impl_->method_; }
std::string &http_request::body() { return impl_->body_; }
const std::string &http_request::body() const { return impl_->body_; }
std::map<std::string, std::string> &http_request::headers() { return impl_->headers_; }
const std::map<std::string, std::string> &http_request::headers() const { return impl_->headers_; }
int &http_request::timeout_ms() { return impl_->timeout_ms_; }
const int &http_request::timeout_ms() const { return impl_->timeout_ms_; }

}
