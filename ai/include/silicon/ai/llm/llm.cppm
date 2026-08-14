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
export import silicon.ai.llm.types;
export import silicon.ai.llm.interface;

export namespace silicon::ai::llm {

// ── 具体实现（DI 就绪，仅依赖接口子模块 + 值类型子模块） ─────────

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
