module;

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>

#include <tuple>

#include <silicon/proxy/proxy_macros.h>
#include <silicon/common.h>

export module silicon.http;

export import silicon.http.error;
export import silicon.http.types;

import silicon.proxy;

export namespace silicon::http {

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

inline http_response get(const http_client_view &client, const std::string &url) {
    http_request req;
    req.url() = url;
    return client->request(req);
}

class SILICON_CORE_API curl_http_client {
  public:
    http_response request(const http_request &) const;
};

class SILICON_CORE_API fake_http_client {

  private:
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

}

// 非导出：平台接缝——读管道执行外部命令的封装。
// 实现位于 http_unix.cpp（popen/pclose）与 http_win.cpp（_popen/_pclose）。
namespace silicon::http {

std::FILE *http_popen_read(const std::string &command);
void http_pclose(std::FILE *pipe);

}
