module;

#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module silicon.ai.llm.interface;

export import silicon.ai.llm.types;

export namespace silicon::ai::llm {

// ── 接口（纯抽象，供 DI 仅依赖此子模块，零 concrete 耦合） ─────────

class i_provider {
  public:
    virtual ~i_provider() = default;
    virtual result<chat_response> chat(const conversation &conv, const model_request_options &opts) = 0;
};

class i_protocol_adapter {
  public:
    virtual ~i_protocol_adapter() = default;
    virtual std::string encode_request(const conversation &conv, const model_request_options &opts, const std::vector<std::string> &tool_defs) const = 0;
    virtual result<chat_response> decode_response(std::string_view raw) const = 0;
};

class i_tool {
  public:
    virtual ~i_tool() = default;
    virtual std::string_view name() const = 0;
    virtual std::string_view description() const = 0;
    virtual tool_output execute(const tool_call &call) = 0;
};

class i_tool_registry {
  public:
    virtual ~i_tool_registry() = default;
    virtual bool register_tool(std::unique_ptr<i_tool> tool) = 0;
    virtual i_tool *get_tool(std::string_view name) const = 0;
    virtual std::size_t tool_count() const = 0;
};

class i_provider_registry {
  public:
    virtual ~i_provider_registry() = default;
    virtual bool register_provider(std::string id, std::unique_ptr<i_provider> provider) = 0;
    virtual i_provider *get_provider(std::string_view id) const = 0;
    virtual std::vector<std::string> list_providers() const = 0;
};

} // namespace silicon::ai::llm
