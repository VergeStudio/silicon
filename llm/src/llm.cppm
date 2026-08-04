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

    struct Impl {
      public:
        std::string role_; // "user" / "assistant" / "system" / "tool"
        std::string content_;
        std::string tool_call_id_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    Message() = default;
    /// 便利构造：保留原聚合初始化 `Message{"user", "hi"}` 的调用形态
    Message(std::string r, std::string c, std::string tcid = {}) {
        impl_->role_ = std::move(r);
        impl_->content_ = std::move(c);
        impl_->tool_call_id_ = std::move(tcid);
    }
    Message(const Message &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    Message &operator=(const Message &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    Message(Message &&) noexcept = default;
    Message &operator=(Message &&) noexcept = default;

  public:
    std::string &Role() { return impl_->role_; }
    const std::string &Role() const { return impl_->role_; }
    std::string &Content() { return impl_->content_; }
    const std::string &Content() const { return impl_->content_; }
    std::string &ToolCallId() { return impl_->tool_call_id_; }
    const std::string &ToolCallId() const { return impl_->tool_call_id_; }

};

using Conversation = std::vector<Message>;

struct ModelRequestOptions {

    struct Impl {
      public:
        std::string model_;
        double temperature_ = 0.7;
        int32_t max_tokens_ = 4096;
        std::map<std::string, std::string, std::less<>> extra_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    ModelRequestOptions() = default;
    ModelRequestOptions(const ModelRequestOptions &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    ModelRequestOptions &operator=(const ModelRequestOptions &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    ModelRequestOptions(ModelRequestOptions &&) noexcept = default;
    ModelRequestOptions &operator=(ModelRequestOptions &&) noexcept = default;

  public:
    std::string &Model() { return impl_->model_; }
    const std::string &Model() const { return impl_->model_; }
    double &Temperature() { return impl_->temperature_; }
    const double &Temperature() const { return impl_->temperature_; }
    int32_t &MaxTokens() { return impl_->max_tokens_; }
    const int32_t &MaxTokens() const { return impl_->max_tokens_; }
    std::map<std::string, std::string, std::less<>> &Extra() { return impl_->extra_; }
    const std::map<std::string, std::string, std::less<>> &Extra() const { return impl_->extra_; }

};

struct ChatResponse {

    struct Impl {
      public:
        std::string content_;
        std::string finish_reason_; // "stop" / "length" / "tool_calls"
        int32_t prompt_tokens_ = 0;
        int32_t completion_tokens_ = 0;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    ChatResponse() = default;
    ChatResponse(const ChatResponse &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    ChatResponse &operator=(const ChatResponse &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    ChatResponse(ChatResponse &&) noexcept = default;
    ChatResponse &operator=(ChatResponse &&) noexcept = default;

  public:
    std::string &Content() { return impl_->content_; }
    const std::string &Content() const { return impl_->content_; }
    std::string &FinishReason() { return impl_->finish_reason_; }
    const std::string &FinishReason() const { return impl_->finish_reason_; }
    int32_t &PromptTokens() { return impl_->prompt_tokens_; }
    const int32_t &PromptTokens() const { return impl_->prompt_tokens_; }
    int32_t &CompletionTokens() { return impl_->completion_tokens_; }
    const int32_t &CompletionTokens() const { return impl_->completion_tokens_; }

};

template<typename T>
using Result = silicon::common::Result<T, silicon::exception::LLMError>;

// ── 接口 ─────────────────────────────────────────────────────────

class IProvider {
  public:
    virtual ~IProvider() = default;
    virtual Result<ChatResponse> Chat(const Conversation &conv, const ModelRequestOptions &opts) = 0;
};

class IProtocolAdapter {
  public:
    virtual ~IProtocolAdapter() = default;
    virtual std::string EncodeRequest(const Conversation &conv, const ModelRequestOptions &opts, const std::vector<std::string> &tool_defs) const = 0;
    virtual Result<ChatResponse> DecodeResponse(std::string_view raw) const = 0;
};

// ── ITool ────────────────────────────────────────────────────────

struct ToolCall {

    struct Impl {
      public:
        std::string id_;
        std::string name_;
        std::string arguments_; // JSON string
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    ToolCall() = default;
    /// 便利构造：保留原聚合初始化 `ToolCall{"1", "echo", "{}"}` 的调用形态
    ToolCall(std::string i, std::string n, std::string args = {}) {
        impl_->id_ = std::move(i);
        impl_->name_ = std::move(n);
        impl_->arguments_ = std::move(args);
    }
    ToolCall(const ToolCall &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    ToolCall &operator=(const ToolCall &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    ToolCall(ToolCall &&) noexcept = default;
    ToolCall &operator=(ToolCall &&) noexcept = default;

  public:
    std::string &Id() { return impl_->id_; }
    const std::string &Id() const { return impl_->id_; }
    std::string &Name() { return impl_->name_; }
    const std::string &Name() const { return impl_->name_; }
    std::string &Arguments() { return impl_->arguments_; }
    const std::string &Arguments() const { return impl_->arguments_; }

};

struct ToolOutput {

    struct Impl {
      public:
        std::string content_;
        bool truncated_ = false;
        std::string managed_output_path_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    ToolOutput() = default;
    ToolOutput(const ToolOutput &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    ToolOutput &operator=(const ToolOutput &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    ToolOutput(ToolOutput &&) noexcept = default;
    ToolOutput &operator=(ToolOutput &&) noexcept = default;

  public:
    std::string &Content() { return impl_->content_; }
    const std::string &Content() const { return impl_->content_; }
    bool &Truncated() { return impl_->truncated_; }
    const bool &Truncated() const { return impl_->truncated_; }
    std::string &ManagedOutputPath() { return impl_->managed_output_path_; }
    const std::string &ManagedOutputPath() const { return impl_->managed_output_path_; }

};

class ITool {
  public:
    virtual ~ITool() = default;
    virtual std::string_view Name() const = 0;
    virtual std::string_view Description() const = 0;
    virtual ToolOutput Execute(const ToolCall &call) = 0;
};

class IToolRegistry {
  public:
    virtual ~IToolRegistry() = default;
    virtual bool RegisterTool(std::unique_ptr<ITool> tool) = 0;
    virtual ITool *GetTool(std::string_view name) const = 0;
    virtual std::size_t ToolCount() const = 0;
};

// ── IProvider 注册表接口 ───────────────────────────────────────

class IProviderRegistry {
  public:
    virtual ~IProviderRegistry() = default;
    virtual bool RegisterProvider(std::string id, std::unique_ptr<IProvider> provider) = 0;
    virtual IProvider *GetProvider(std::string_view id) const = 0;
    virtual std::vector<std::string> ListProviders() const = 0;
};

// ── 具体实现（DI 就绪，仅依赖接口） ─────────────────────────────

/// 内存工具注册表：重复 name 注册返回 false（不替换）。
class ToolRegistry: public IToolRegistry {

    struct Impl {
      public:
      std::map<std::string, std::unique_ptr<ITool>, std::less<>> tools_;
    };
    std::unique_ptr<Impl> impl_;

  public:
    ToolRegistry();
    bool RegisterTool(std::unique_ptr<ITool> tool) override;
    ITool *GetTool(std::string_view name) const override;
    std::size_t ToolCount() const override;

};

/// 内存提供方注册表：重复 id 注册返回 false。
class ProviderRegistry: public IProviderRegistry {

    struct Impl {
      public:
      std::map<std::string, std::unique_ptr<IProvider>, std::less<>> providers_;
    };
    std::unique_ptr<Impl> impl_;

  public:
    ProviderRegistry();
    bool RegisterProvider(std::string id, std::unique_ptr<IProvider> provider) override;
    IProvider *GetProvider(std::string_view id) const override;
    std::vector<std::string> ListProviders() const override;

};

/// OpenAI 风格 JSON 协议适配器：Conversation/Options -> 请求 JSON；
/// 线路 JSON -> ChatResponse（choices[0].message.content 等）。
class JsonProtocolAdapter: public IProtocolAdapter {
  public:
    std::string EncodeRequest(
            const Conversation &conv,
            const ModelRequestOptions &opts,
            const std::vector<std::string> &tool_defs
    ) const override;
    Result<ChatResponse> DecodeResponse(std::string_view raw) const override;
};

/// 脚本化提供方：FIFO 返回预置响应，用于确定性 TDD。
/// 队列耗尽返回 LLMError，绝不抛异常。
class ScriptedProvider: public IProvider {

    struct Impl {
      public:
      std::queue<ChatResponse> queue_;
    };
    std::unique_ptr<Impl> impl_;

  public:
    ScriptedProvider();
    void Enqueue(ChatResponse r);
    std::size_t Remaining() const;

    Result<ChatResponse> Chat(const Conversation &, const ModelRequestOptions &) override;

};

/// OpenAI 兼容 HTTP IProvider：通过本地 curl 调用 {base_url}/chat/completions。
/// 配置来自环境变量（无 key 时 chat 返回 LLMError，由调用方提示用户）。
/// 选用 OpenAI 兼容协议，可对接 OpenAI / DeepSeek / Ollama / vLLM / LM Studio 等。
class HttpProvider: public IProvider {

    struct Impl {
      public:
        std::string base_url_;
        std::string api_key_;
        std::string model_;
        JsonProtocolAdapter adapter_;
    };
    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};

    static std::string EnvOr(const char *name, std::string def);

    struct HttpResult {

        struct Impl {
          public:
            int status_ = 0;
            std::string body_;
        };
        std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

      public:
        HttpResult() = default;
        HttpResult(const HttpResult &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
        HttpResult &operator=(const HttpResult &o) {
            if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
            return *this;
        }
        HttpResult(HttpResult &&) noexcept = default;
        HttpResult &operator=(HttpResult &&) noexcept = default;

      public:
        int &Status() { return impl_->status_; }
        const int &Status() const { return impl_->status_; }
        std::string &Body() { return impl_->body_; }
        const std::string &Body() const { return impl_->body_; }
    };

    // 用临时文件承载请求体，避开 JSON 中的引号转义问题；跨平台用 -H 传头。
    HttpResult PostJson(const std::string &url, const std::string &body) const;

  public:
    HttpProvider();
    bool Configured() const;
    std::string_view ModelName() const;
    Result<ChatResponse> Chat(const Conversation &conv, const ModelRequestOptions &opts) override;
};

} // namespace silicon::llm
