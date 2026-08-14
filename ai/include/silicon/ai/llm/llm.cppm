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

import silicon.json;
import silicon.core;

export module silicon.ai.llm;
export import silicon.ai.llm.error;

export namespace silicon::ai::llm {


// ── 值类型 ──────────────────────────────────────────────────────

struct message {

    struct impl;                 // 完整定义下沉至 llm.cpp（message 非模版）
    std::shared_ptr<impl> impl_;

  public:
    message();
    /// 便利构造：保留原聚合初始化 `message{"user", "hi"}` 的调用形态
    message(std::string r, std::string c, std::string tcid = {});
    message(const message &o);
    message &operator=(const message &o);
    message(message &&) noexcept;
    message &operator=(message &&) noexcept;

  public:
    std::string &role();
    const std::string &role() const;
    std::string &content();
    const std::string &content() const;
    std::string &tool_call_id();
    const std::string &tool_call_id() const;

};

using conversation = std::vector<message>;

struct model_request_options {

    struct impl;                 // 完整定义下沉至 llm.cpp（model_request_options 非模版）
    std::shared_ptr<impl> impl_;

  public:
    model_request_options();
    model_request_options(const model_request_options &o);
    model_request_options &operator=(const model_request_options &o);
    model_request_options(model_request_options &&) noexcept;
    model_request_options &operator=(model_request_options &&) noexcept;

  public:
    std::string &model();
    const std::string &model() const;
    double &temperature();
    const double &temperature() const;
    int32_t &max_tokens();
    const int32_t &max_tokens() const;
    std::map<std::string, std::string, std::less<>> &extra();
    const std::map<std::string, std::string, std::less<>> &extra() const;

};

struct chat_response {

    struct impl;                 // 完整定义下沉至 llm.cpp（chat_response 非模版）
    std::shared_ptr<impl> impl_; // 接口单元中 impl 不完整；默认构造/拷贝/访问器均在 .cpp 定义

  public:
    chat_response();
    chat_response(const chat_response &o);
    chat_response &operator=(const chat_response &o);
    chat_response(chat_response &&) noexcept;
    chat_response &operator=(chat_response &&) noexcept;

  public:
    std::string &content();
    const std::string &content() const;
    std::string &finish_reason();
    const std::string &finish_reason() const;
    int32_t &prompt_tokens();
    const int32_t &prompt_tokens() const;
    int32_t &completion_tokens();
    const int32_t &completion_tokens() const;

};

template<typename T>
using result = std::expected<T, std::error_code>;

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

    struct impl;                 // 完整定义下沉至 llm.cpp（tool_call 非模版）
    std::shared_ptr<impl> impl_;

  public:
    tool_call();
    /// 便利构造：保留原聚合初始化 `tool_call{"1", "echo", "{}"}` 的调用形态
    tool_call(std::string i, std::string n, std::string args = {});
    tool_call(const tool_call &o);
    tool_call &operator=(const tool_call &o);
    tool_call(tool_call &&) noexcept;
    tool_call &operator=(tool_call &&) noexcept;

  public:
    std::string &id();
    const std::string &id() const;
    std::string &name();
    const std::string &name() const;
    std::string &arguments();
    const std::string &arguments() const;

};

struct tool_output {

    struct impl;                 // 完整定义下沉至 llm.cpp（tool_output 非模版）
    std::shared_ptr<impl> impl_;

  public:
    tool_output();
    tool_output(const tool_output &o);
    tool_output &operator=(const tool_output &o);
    tool_output(tool_output &&) noexcept;
    tool_output &operator=(tool_output &&) noexcept;

  public:
    std::string &content();
    const std::string &content() const;
    bool &truncated();
    const bool &truncated() const;
    std::string &managed_output_path();
    const std::string &managed_output_path() const;

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

    struct impl {
      public:
      std::map<std::string, std::unique_ptr<i_tool>, std::less<>> tools_;
    };
    std::unique_ptr<impl> impl_;

  public:
    tool_registry();
    bool register_tool(std::unique_ptr<i_tool> tool) override;
    i_tool *get_tool(std::string_view name) const override;
    std::size_t tool_count() const override;

};

/// 内存提供方注册表：重复 id 注册返回 false。
class provider_registry: public i_provider_registry {

    struct impl {
      public:
      std::map<std::string, std::unique_ptr<i_provider>, std::less<>> providers_;
    };
    std::unique_ptr<impl> impl_;

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

    struct impl {
      public:
      std::queue<chat_response> queue_;
    };
    std::unique_ptr<impl> impl_;

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
    result<chat_response> chat(const conversation &conv, const model_request_options &opts) override;
};

} // namespace silicon::ai::llm
