module;

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

import silicon.json;
import silicon.core;
import silicon.exception;

export module silicon.llm;

export namespace silicon::llm {

// ── 值类型 ──────────────────────────────────────────────────────

struct Message {
    std::string role; // "user" / "assistant" / "system" / "tool"
    std::string content;
    std::string tool_call_id;
};

using Conversation = std::vector<Message>;

struct ModelRequestOptions {
    std::string model;
    double temperature = 0.7;
    int32_t max_tokens = 4096;
    std::map<std::string, std::string, std::less<>> extra;
};

struct ChatResponse {
    std::string content;
    std::string finish_reason; // "stop" / "length" / "tool_calls"
    int32_t prompt_tokens = 0;
    int32_t completion_tokens = 0;
};

template<typename T>
using Result = silicon::common::Result<T, silicon::exception::LLMError>;

// ── I* 接口 ─────────────────────────────────────────────────────

class IProvider {
  public:
    virtual ~IProvider() = default;
    virtual Result<ChatResponse> chat(const Conversation &conv, const ModelRequestOptions &opts) = 0;
};

class IProtocolAdapter {
  public:
    virtual ~IProtocolAdapter() = default;
    virtual std::string encode_request(const Conversation &conv, const ModelRequestOptions &opts, const std::vector<std::string> &tool_defs) const = 0;
    virtual Result<ChatResponse> decode_response(std::string_view raw) const = 0;
};

// ── Tool ────────────────────────────────────────────────────────

struct ToolCall {
    std::string id;
    std::string name;
    std::string arguments; // JSON string
};

struct ToolOutput {
    std::string content;
    bool truncated = false;
    std::string managed_output_path;
};

class ITool {
  public:
    virtual ~ITool() = default;
    virtual std::string_view name() const = 0;
    virtual std::string_view description() const = 0;
    virtual ToolOutput execute(const ToolCall &call) = 0;
};

class IToolRegistry {
  public:
    virtual ~IToolRegistry() = default;
    virtual bool register_tool(std::unique_ptr<ITool> tool) = 0;
    virtual ITool *get_tool(std::string_view name) const = 0;
    virtual std::size_t tool_count() const = 0;
};

// ── Provider 注册表接口 ───────────────────────────────────────

class IProviderRegistry {
  public:
    virtual ~IProviderRegistry() = default;
    virtual bool register_provider(std::string id, std::unique_ptr<IProvider> provider) = 0;
    virtual IProvider *get_provider(std::string_view id) const = 0;
    virtual std::vector<std::string> list_providers() const = 0;
};

// ── 具体实现（DI 就绪，仅依赖接口） ─────────────────────────────

/// 内存工具注册表：重复 name 注册返回 false（不替换）。
class ToolRegistry: public IToolRegistry {
    std::map<std::string, std::unique_ptr<ITool>, std::less<>> tools_;

  public:
    bool register_tool(std::unique_ptr<ITool> tool) override;
    ITool *get_tool(std::string_view name) const override;
    std::size_t tool_count() const override;
};

/// 内存提供方注册表：重复 id 注册返回 false。
class ProviderRegistry: public IProviderRegistry {
    std::map<std::string, std::unique_ptr<IProvider>, std::less<>> providers_;

  public:
    bool register_provider(std::string id, std::unique_ptr<IProvider> provider) override;
    IProvider *get_provider(std::string_view id) const override;
    std::vector<std::string> list_providers() const override;
};

/// OpenAI 风格 JSON 协议适配器：Conversation/Options -> 请求 JSON；
/// 线路 JSON -> ChatResponse（choices[0].message.content 等）。
class JsonProtocolAdapter: public IProtocolAdapter {
  public:
    std::string encode_request(
            const Conversation &conv,
            const ModelRequestOptions &opts,
            const std::vector<std::string> &tool_defs
    ) const override;
    Result<ChatResponse> decode_response(std::string_view raw) const override;
};

/// 脚本化提供方：FIFO 返回预置响应，用于确定性 TDD。
/// 队列耗尽返回 LLMError，绝不抛异常。
class ScriptedProvider: public IProvider {
    std::queue<ChatResponse> queue_;

  public:
    void enqueue(ChatResponse r);
    std::size_t remaining() const;

    Result<ChatResponse> chat(const Conversation &, const ModelRequestOptions &) override;
};

/// OpenAI 兼容 HTTP Provider：通过本地 curl 调用 {base_url}/chat/completions。
/// 配置来自环境变量（无 key 时 chat 返回 LLMError，由调用方提示用户）。
/// 选用 OpenAI 兼容协议，可对接 OpenAI / DeepSeek / Ollama / vLLM / LM Studio 等。
class HttpProvider: public IProvider {
    std::string base_url_;
    std::string api_key_;
    std::string model_;
    JsonProtocolAdapter adapter_;

    static std::string env_or(const char *name, std::string def);

    struct HttpResult {
        int status = 0;
        std::string body;
    };

    // 用临时文件承载请求体，避开 JSON 中的引号转义问题；跨平台用 -H 传头。
    HttpResult post_json(const std::string &url, const std::string &body) const;

  public:
    HttpProvider();
    bool configured() const;
    std::string_view model_name() const;
    Result<ChatResponse> chat(const Conversation &conv, const ModelRequestOptions &opts) override;
};

} // namespace silicon::llm
