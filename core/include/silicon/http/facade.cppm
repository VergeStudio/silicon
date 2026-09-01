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
// http_response / http_request 为 header-only（global module 实体），跨工具链
// mangling 兼容（MSVC 命名模块符号带 ::<!module> 标签而 clang 不带），见
// http_types.h 内注释。必须在全局模块片段文本包含，不随模块导出。
#include <silicon/http/http_types.h>
export module silicon.http;

export import silicon.http.error;


import silicon.proxy;

export namespace silicon::http {

// http_response / http_request 已抽为 header-only（global module 实体），见
// silicon/http/http_types.h：跨工具链（MSVC 编 core.dll + clang 消费）时，
// 命名模块的 mangling 标签与 IFC 不含函数体会导致 clang 对成员函数发强引用、
// 链接未定义；header-only 让每个消费 TU 本地发射 weak 符号，彻底规避。
// 本模块（及消费方）须 #include <silicon/http/http_types.h> 以拿到这两个类型。

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
