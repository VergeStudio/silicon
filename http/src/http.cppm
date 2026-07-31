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
    int status_code = 0;
    std::string body;
    std::map<std::string, std::string> headers;
};

struct HttpRequest {
    std::string url;
    std::string method = "GET";
    std::string body;
    std::map<std::string, std::string> headers;
    int timeout_ms = 30000;
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
    std::unique_ptr<P> m_p;

  public:
    explicit FakeHttpClient(HttpResponse response = {200, "{}"});
    HttpResponse request(const HttpRequest &) const override;
    std::size_t call_count() const;

};

} // namespace silicon::http
