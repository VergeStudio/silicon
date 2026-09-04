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

#include <silicon/proxy/proxy_macros.h>
#include <silicon/common.h>

#include <silicon/http/http_types.h>
export module silicon.http;

export import silicon.http.error;

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

class CORE_API curl_http_client {
  public:
    http_response request(const http_request &) const;
};

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

}
