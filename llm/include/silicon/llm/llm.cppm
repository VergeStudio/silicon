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

struct message {

    struct Impl {
      public:
        std::string role_; // "user" / "assistant" / "system" / "tool"
        std::string content_;
        std::string tool_call_id_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    message() = default;
    /// 便利构造：保留原聚合初始化 `message{"user", "hi"}` 的调用形态
    message(std::string r, std::string c, std::string tcid = {}) {
        impl_->role_ = std::move(r);
        impl_->content_ = std::move(c);
        impl_->tool_call_id_ = std::move(tcid);
    }
    message(const message &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    message &operator=(const message &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    message(message &&) noexcept = default;
    message &operator=(message &&) noexcept = default;

  public:
    std::string &role() { return impl_->role_; }
    const std::string &role() const { return impl_->role_; }
    std::string &content() { return impl_->content_; }
    const std::string &content() const { return impl_->content_; }
    std::string &tool_call_id() { return impl_->tool_call_id_; }
    const std::string &tool_call_id() const { return impl_->tool_call_id_; }

};

using conversation = std::vector<message>;

struct model_request_options {

    struct Impl {
      public:
        std::string model_;
        double temperature_ = 0.7;
        int32_t max_tokens_ = 4096;
        std::map<std::string, std::string, std::less<>> extra_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    model_request_options() = default;
    model_request_options(const model_request_options &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    model_request_options &operator=(const model_request_options &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    model_request_options(model_request_options &&) noexcept = default;
    model_request_options &operator=(model_request_options &&) noexcept = default;

  public:
    std::string &model() { return impl_->model_; }
    const std::string &model() const { return impl_->model_; }
    double &temperature() { return impl_->temperature_; }
    const double &temperature() const { return impl_->temperature_; }
    int32_t &max_tokens() { return impl_->max_tokens_; }
    const int32_t &max_tokens() const { return impl_->max_tokens_; }
    std::map<std::string, std::string, std::less<>> &extra() { return impl_->extra_; }
    const std::map<std::string, std::string, std::less<>> &extra() const { return impl_->extra_; }

};

struct chat_response {

    struct Impl {
      public:
        std::string content_;
        std::string finish_reason_; // "stop" / "length" / "tool_calls"
        int32_t prompt_tokens_ = 0;
        int32_t completion_tokens_ = 0;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    chat_response() = default;
    chat_response(const chat_response &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    chat_response &operator=(const chat_response &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    chat_response(chat_response &&) noexcept = default;
    chat_response &operator=(chat_response &&) noexcept = default;

  public:
    std::string &content() { return impl_->content_; }
    const std::string &content() const { return impl_->content_; }
    std::string &finish_reason() { return impl_->finish_reason_; }
    const std::string &finish_reason() const { return impl_->finish_reason_; }
    int32_t &prompt_tokens() { return impl_->prompt_tokens_; }
    const int32_t &prompt_tokens() const { return impl_->prompt_tokens_; }
    int32_t &completion_tokens() { return impl_->completion_tokens_; }
    const int32_t &completion_tokens() const { return impl_->completion_tokens_; }

};

template<typename T>
using result = silicon::common::result<T, silicon::exception::llm_error>;

// ── 接口 ─────────────────────────────────────────────────────────

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

// ── i_tool ────────────────────────────────────────────────────────

struct tool_call {

    struct Impl {
      public:
        std::string id_;
        std::string name_;
        std::string arguments_; // JSON string
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    tool_call() = default;
    /// 便利构造：保留原聚合初始化 `tool_call{"1", "echo", "{}"}` 的调用形态
    tool_call(std::string i, std::string n, std::string args = {}) {
        impl_->id_ = std::move(i);
        impl_->name_ = std::move(n);
        impl_->arguments_ = std::move(args);
    }
    tool_call(const tool_call &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    tool_call &operator=(const tool_call &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    tool_call(tool_call &&) noexcept = default;
    tool_call &operator=(tool_call &&) noexcept = default;

  public:
    std::string &id() { return impl_->id_; }
    const std::string &id() const { return impl_->id_; }
    std::string &name() { return impl_->name_; }
    const std::string &name() const { return impl_->name_; }
    std::string &arguments() { return impl_->arguments_; }
    const std::string &arguments() const { return impl_->arguments_; }

};

struct tool_output {

    struct Impl {
      public:
        std::string content_;
        bool truncated_ = false;
        std::string managed_output_path_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    tool_output() = default;
    tool_output(const tool_output &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    tool_output &operator=(const tool_output &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    tool_output(tool_output &&) noexcept = default;
    tool_output &operator=(tool_output &&) noexcept = default;

  public:
    std::string &content() { return impl_->content_; }
    const std::string &content() const { return impl_->content_; }
    bool &truncated() { return impl_->truncated_; }
    const bool &truncated() const { return impl_->truncated_; }
    std::string &managed_output_path() { return impl_->managed_output_path_; }
    const std::string &managed_output_path() const { return impl_->managed_output_path_; }

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

// ── i_provider 注册表接口 ───────────────────────────────────────

class i_provider_registry {
  public:
    virtual ~i_provider_registry() = default;
    virtual bool register_provider(std::string id, std::unique_ptr<i_provider> provider) = 0;
    virtual i_provider *get_provider(std::string_view id) const = 0;
    virtual std::vector<std::string> list_providers() const = 0;
};

// ── 具体实现（DI 就绪，仅依赖接口） ─────────────────────────────

/// 内存工具注册表：重复 name 注册返回 false（不替换）。
class tool_registry: public i_tool_registry {

    struct Impl {
      public:
      std::map<std::string, std::unique_ptr<i_tool>, std::less<>> tools_;
    };
    std::unique_ptr<Impl> impl_;

  public:
    tool_registry();
    bool register_tool(std::unique_ptr<i_tool> tool) override;
    i_tool *get_tool(std::string_view name) const override;
    std::size_t tool_count() const override;

};

/// 内存提供方注册表：重复 id 注册返回 false。
class provider_registry: public i_provider_registry {

    struct Impl {
      public:
      std::map<std::string, std::unique_ptr<i_provider>, std::less<>> providers_;
    };
    std::unique_ptr<Impl> impl_;

  public:
    provider_registry();
    bool register_provider(std::string id, std::unique_ptr<i_provider> provider) override;
    i_provider *get_provider(std::string_view id) const override;
    std::vector<std::string> list_providers() const override;

};

/// OpenAI 风格 JSON 协议适配器：conversation/Options -> 请求 JSON；
/// 线路 JSON -> chat_response（choices[0].message.content 等）。
class json_protocol_adapter: public i_protocol_adapter {
  public:
    std::string encode_request(
            const conversation &conv,
            const model_request_options &opts,
            const std::vector<std::string> &tool_defs
    ) const override;
    result<chat_response> decode_response(std::string_view raw) const override;
};

/// 脚本化提供方：FIFO 返回预置响应，用于确定性 TDD。
/// 队列耗尽返回 llm_error，绝不抛异常。
class scripted_provider: public i_provider {

    struct Impl {
      public:
      std::queue<chat_response> queue_;
    };
    std::unique_ptr<Impl> impl_;

  public:
    scripted_provider();
    void enqueue(chat_response r);
    std::size_t remaining() const;

    result<chat_response> chat(const conversation &, const model_request_options &) override;

};

/// OpenAI 兼容 HTTP i_provider：通过本地 curl 调用 {base_url}/chat/completions。
/// 配置来自环境变量（无 key 时 chat 返回 llm_error，由调用方提示用户）。
/// 选用 OpenAI 兼容协议，可对接 OpenAI / DeepSeek / Ollama / vLLM / LM Studio 等。
class http_provider: public i_provider {

    struct Impl {
      public:
        std::string base_url_;
        std::string api_key_;
        std::string model_;
        json_protocol_adapter adapter_;
    };
    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};

    static std::string env_or(const char *name, std::string def);

    struct http_result {

        struct Impl {
          public:
            int status_ = 0;
            std::string body_;
        };
        std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

      public:
        http_result() = default;
        http_result(const http_result &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
        http_result &operator=(const http_result &o) {
            if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
            return *this;
        }
        http_result(http_result &&) noexcept = default;
        http_result &operator=(http_result &&) noexcept = default;

      public:
        int &status() { return impl_->status_; }
        const int &status() const { return impl_->status_; }
        std::string &body() { return impl_->body_; }
        const std::string &body() const { return impl_->body_; }
    };

    // 用临时文件承载请求体，避开 JSON 中的引号转义问题；跨平台用 -H 传头。
    http_result post_json(const std::string &url, const std::string &body) const;

  public:
    http_provider();
    bool configured() const;
    std::string_view model_name() const;
    result<chat_response> chat(const conversation &conv, const model_request_options &opts) override;
};

} // namespace silicon::llm
