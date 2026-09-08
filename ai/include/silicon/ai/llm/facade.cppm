module;

#include <cstdint>
#include <expected>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <silicon/proxy/proxy_macros.h>

#include <tuple>

#include <silicon/ai/common.h>
export module silicon.ai.llm;

import silicon.json;
import silicon.core;
import silicon.proxy;
export import silicon.ai.llm.error;
export import silicon.ai.llm.types;

export namespace silicon::ai::llm {

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

struct tool_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemToolName, std::string_view() const>
      ::add_convention<MemToolDescription, std::string_view() const>
      ::add_convention<MemToolExecute, tool_output(tool_call)>
      ::support_copy<silicon::proxy::constraint_level::kNontrivial>
      ::build {};

struct provider_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemProviderChat, silicon::error::result<chat_response>(const conversation &, const model_request_options &)>
      ::support_copy<silicon::proxy::constraint_level::kNontrivial>
      ::build {};

struct protocol_adapter_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemAdapterEncode, std::string(const conversation &, const model_request_options &, const std::vector<std::string> &) const>
      ::add_convention<MemAdapterDecode, silicon::error::result<chat_response>(std::string_view) const>
      ::build {};


using tool_view = silicon::proxy::proxy_view<tool_facade>;
using provider_view = silicon::proxy::proxy_view<provider_facade>;

struct tool_registry_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemToolRegRegister, bool(silicon::proxy::proxy<tool_facade>)>
      ::add_convention<MemToolRegGet, silicon::proxy::proxy<tool_facade>(std::string_view) const>
      ::add_convention<MemToolRegCount, std::size_t() const>
      ::build {};

struct provider_registry_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemProviderRegRegister, bool(std::string, silicon::proxy::proxy<provider_facade>)>
      ::add_convention<MemProviderRegGet, silicon::proxy::proxy<provider_facade>(std::string_view) const>
      ::add_convention<MemProviderRegList, std::vector<std::string>() const>
      ::build {};

template<class T, class... Args>
[[nodiscard]] silicon::proxy::proxy<tool_facade> make_tool(Args &&...args) {
    return silicon::proxy::make_proxy<tool_facade, T>(std::forward<Args>(args)...);
}
template<class T, class... Args>
[[nodiscard]] silicon::proxy::proxy<provider_facade> make_provider(Args &&...args) {
    return silicon::proxy::make_proxy<provider_facade, T>(std::forward<Args>(args)...);
}
template<class T, class... Args>
[[nodiscard]] silicon::proxy::proxy<protocol_adapter_facade> make_adapter(Args &&...args) {
    return silicon::proxy::make_proxy<protocol_adapter_facade, T>(std::forward<Args>(args)...);
}

class AI_API tool_registry {

  private:
    struct impl;
    std::unique_ptr<impl> impl_;

  public:
    tool_registry();
    ~tool_registry();
    bool register_tool(silicon::proxy::proxy<tool_facade>);
    silicon::proxy::proxy<tool_facade> get_tool(std::string_view) const;
    std::size_t tool_count() const;
};

class AI_API provider_registry {

  private:
    struct impl;
    std::unique_ptr<impl> impl_;

  public:
    provider_registry();
    ~provider_registry();
    bool register_provider(std::string, silicon::proxy::proxy<provider_facade>);
    silicon::proxy::proxy<provider_facade> get_provider(std::string_view) const;
    std::vector<std::string> list_providers() const;
};

class AI_API json_protocol_adapter {
  public:
    std::string encode_request(
            const conversation &,
            const model_request_options &,
            const std::vector<std::string> &
    ) const;
    silicon::error::result<chat_response> decode_response(std::string_view) const;
};

class AI_API scripted_provider {

  private:
    struct impl;
    std::unique_ptr<impl> impl_;

  public:
    scripted_provider();
    ~scripted_provider();
    void enqueue(chat_response);
    std::size_t remaining() const;

    silicon::error::result<chat_response> chat(const conversation &, const model_request_options &);
};

class AI_API http_provider {

  private:
    struct impl;
    std::unique_ptr<impl> impl_;

    static std::string env_or(const char *, std::string);

    struct http_result {

      private:
        struct impl;
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

    http_result post_json(const std::string &, const std::string &) const;

  public:
    http_provider();
    ~http_provider();
    bool configured() const;
    std::string_view model_name() const;
    silicon::error::result<chat_response> chat(const conversation &, const model_request_options &);
};

}
