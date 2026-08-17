module;

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <expected>

// proxy 的 dispatch 宏走头文件通道，本模块定义门面须在全局模块片段显式
// 包含，随后再 import silicon.proxy（宏不随 C++20 模块导出）。
#include <silicon/proxy/proxy_macros.h>

import silicon.json;
import silicon.core;
import silicon.proxy;

export module silicon.ai.llm;
export import silicon.ai.llm.error;
export import silicon.ai.llm.types;

export namespace silicon::ai::llm {

// ── 类型擦除门面（silicon.proxy）─────────────
//
// 目标类型无需继承任何基类，只要拥有匹配签名的成员即自动满足门面（鸭子
// 类型）。既有的具体类 scripted_provider / http_provider /
// json_protocol_adapter，以及测试中的 EchoTool / ConstProvider 均直接接入，
// 不再耦合任何继承体系。跨 DLL/ABI 边界以「胖指针 + vtable 值」替代虚表。

PRO_DEF_MEM_DISPATCH(MemToolName, name);
PRO_DEF_MEM_DISPATCH(MemToolDescription, description);
PRO_DEF_MEM_DISPATCH(MemToolExecute, execute);
PRO_DEF_MEM_DISPATCH(MemProviderChat, chat);
PRO_DEF_MEM_DISPATCH(MemAdapterEncode, encode_request);
PRO_DEF_MEM_DISPATCH(MemAdapterDecode, decode_response);
PRO_DEF_MEM_DISPATCH(MemToolRegRegister, register_tool);
PRO_DEF_MEM_DISPATCH(MemToolRegGet, get_tool);
PRO_DEF_MEM_DISPATCH(MemToolRegCount, tool_count);
PRO_DEF_MEM_DISPATCH(MemProviderRegRegister, register_provider);
PRO_DEF_MEM_DISPATCH(MemProviderRegGet, get_provider);
PRO_DEF_MEM_DISPATCH(MemProviderRegList, list_providers);

/// 工具门面：满足 `std::string_view name() const` /
/// `std::string_view description() const` / `tool_output execute(tool_call)`。
struct tool_facade
    : silicon::proxy::facade_builder                                                       //
      ::add_convention<MemToolName, std::string_view() const>                             //
      ::add_convention<MemToolDescription, std::string_view() const>                      //
      ::add_convention<MemToolExecute, tool_output(tool_call)>                            //
      ::build {};

/// 提供方门面：满足 `result<chat_response> chat(conversation const&,
/// model_request_options const&)`。
struct provider_facade
    : silicon::proxy::facade_builder                                                       //
      ::add_convention<MemProviderChat, result<chat_response>(const conversation &, const model_request_options &)> //
      ::build {};

/// 协议适配器门面：满足 encode_request / decode_response 两个成员。
struct protocol_adapter_facade
    : silicon::proxy::facade_builder                                                       //
      ::add_convention<MemAdapterEncode, std::string(const conversation &, const model_request_options &, const std::vector<std::string> &) const> //
      ::add_convention<MemAdapterDecode, result<chat_response>(std::string_view) const>   //
      ::build {};

/// 拥有所有权的类型擦除句柄（值语义；小对象内联，无堆分配）。
using tool_proxy = silicon::proxy::proxy<tool_facade>;
using provider_proxy = silicon::proxy::proxy<provider_facade>;
using protocol_adapter_proxy = silicon::proxy::proxy<protocol_adapter_facade>;

/// 非拥有观察视图，等价于裸指针但不要求继承。
using tool_view = silicon::proxy::proxy_view<tool_facade>;
using provider_view = silicon::proxy::proxy_view<provider_facade>;

/// 工具注册表门面：注册/查询/计数，全部以 tool_proxy 承载工具。
struct tool_registry_facade
    : silicon::proxy::facade_builder                                                       //
      ::add_convention<MemToolRegRegister, bool(tool_proxy)>                              //
      ::add_convention<MemToolRegGet, tool_proxy(std::string_view) const>                 //
      ::add_convention<MemToolRegCount, std::size_t() const>                              //
      ::build {};

/// 提供方注册表门面：注册/查询/列举，全部以 provider_proxy 承载提供方。
struct provider_registry_facade
    : silicon::proxy::facade_builder                                                       //
      ::add_convention<MemProviderRegRegister, bool(std::string, provider_proxy)>          //
      ::add_convention<MemProviderRegGet, provider_proxy(std::string_view) const>         //
      ::add_convention<MemProviderRegList, std::vector<std::string>() const>               //
      ::build {};

/// 就地构造任意满足门面的目标类型并擦除为 proxy；句柄按值持有。
template<class T, class... Args>
[[nodiscard]] tool_proxy make_tool(Args &&...args) {
    return silicon::proxy::make_proxy<tool_facade, T>(std::forward<Args>(args)...);
}
template<class T, class... Args>
[[nodiscard]] provider_proxy make_provider(Args &&...args) {
    return silicon::proxy::make_proxy<provider_facade, T>(std::forward<Args>(args)...);
}
template<class T, class... Args>
[[nodiscard]] protocol_adapter_proxy make_adapter(Args &&...args) {
    return silicon::proxy::make_proxy<protocol_adapter_facade, T>(std::forward<Args>(args)...);
}

// ── 具体实现（鸭子类型满足上方门面，零抽象基类耦合） ─────────────

/// 内存工具注册表：重复 name 注册返回 false（不替换）。
class tool_registry {

    struct impl {
      public:
        std::map<std::string, tool_proxy, std::less<>> tools_;
    };
    std::unique_ptr<impl> impl_;

  public:
    tool_registry();
    bool register_tool(tool_proxy tool);
    tool_proxy get_tool(std::string_view name) const;
    std::size_t tool_count() const;

};

/// 内存提供方注册表：重复 id 注册返回 false。
class provider_registry {

    struct impl {
      public:
        std::map<std::string, provider_proxy, std::less<>> providers_;
    };
    std::unique_ptr<impl> impl_;

  public:
    provider_registry();
    bool register_provider(std::string id, provider_proxy provider);
    provider_proxy get_provider(std::string_view id) const;
    std::vector<std::string> list_providers() const;

};

/// OpenAI 风格 JSON 协议适配器：conversation/Options -> 请求 JSON；
/// 线路 JSON -> chat_response（choices[0].message.content 等）。
class json_protocol_adapter {
  public:
    std::string encode_request(
            const conversation &conv,
            const model_request_options &opts,
            const std::vector<std::string> &tool_defs
    ) const;
    result<chat_response> decode_response(std::string_view raw) const;
};

/// 脚本化提供方：FIFO 返回预置响应，用于确定性 TDD。
/// 队列耗尽返回 llm_error，绝不抛异常。
class scripted_provider {

    struct impl {
      public:
      std::queue<chat_response> queue_;
    };
    std::unique_ptr<impl> impl_;

  public:
    scripted_provider();
    void enqueue(chat_response r);
    std::size_t remaining() const;

    result<chat_response> chat(const conversation &, const model_request_options &);

};

/// OpenAI 兼容 HTTP provider：通过本地 curl 调用 {base_url}/chat/completions。
/// 配置来自环境变量（无 key 时 chat 返回 llm_error，由调用方提示用户）。
/// 选用 OpenAI 兼容协议，可对接 OpenAI / DeepSeek / Ollama / vLLM / LM Studio 等。
class http_provider {

    struct impl {
      public:
        std::string base_url_;
        std::string api_key_;
        std::string model_;
        json_protocol_adapter adapter_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

    static std::string env_or(const char *name, std::string def);

    struct http_result {

        struct impl;                 // 完整定义下沉至 llm.cpp（http_result 非模版）
        std::shared_ptr<impl> impl_;

      public:
        http_result();
        http_result(const http_result &o);
        http_result &operator=(const http_result &o);
        http_result(http_result &&) noexcept;
        http_result &operator=(http_result &&) noexcept;

      public:
        int &status();
        const int &status() const;
        std::string &body();
        const std::string &body() const;
    };

    // 用临时文件承载请求体，避开 JSON 中的引号转义问题；跨平台用 -H 传头。
    http_result post_json(const std::string &url, const std::string &body) const;

  public:
    http_provider();
    bool configured() const;
    std::string_view model_name() const;
    result<chat_response> chat(const conversation &conv, const model_request_options &opts);
};

} // namespace silicon::ai::llm
