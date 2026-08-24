module;

#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

export module silicon.ai.llm.types;

import silicon.error;

export namespace silicon::ai::llm {

/// 统一错误返回类型：llm 模块可失败 API 返回 result<T>。
/// 转发至 silicon.error 的集中别名。
template<typename T>
using result = silicon::error::result<T>;

// ── 值类型 ──────────────────────────────────────────────────────

struct message {

    struct impl; // 完整定义下沉至 types.cpp（message 非模版）
    std::shared_ptr<impl> impl_;

  public:
    message();
    /// 便利构造：保留原聚合初始化 `message{"user", "hi"}` 的调用形态
    message(std::string, std::string, std::string = {});
    message(const message &);
    message &operator=(const message &);
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

    struct impl; // 完整定义下沉至 types.cpp（model_request_options 非模版）
    std::shared_ptr<impl> impl_;

  public:
    model_request_options();
    model_request_options(const model_request_options &);
    model_request_options &operator=(const model_request_options &);
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

    struct impl;                 // 完整定义下沉至 types.cpp（chat_response 非模版）
    std::shared_ptr<impl> impl_; // 接口单元中 impl 不完整；默认构造/拷贝/访问器均在 types.cpp 定义

  public:
    chat_response();
    chat_response(const chat_response &);
    chat_response &operator=(const chat_response &);
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

struct tool_call {

    struct impl; // 完整定义下沉至 types.cpp（tool_call 非模版）
    std::shared_ptr<impl> impl_;

  public:
    tool_call();
    /// 便利构造：保留原聚合初始化 `tool_call{"1", "echo", "{}"}` 的调用形态
    tool_call(std::string, std::string, std::string = {});
    tool_call(const tool_call &);
    tool_call &operator=(const tool_call &);
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

    struct impl; // 完整定义下沉至 types.cpp（tool_output 非模版）
    std::shared_ptr<impl> impl_;

  public:
    tool_output();
    tool_output(const tool_output &);
    tool_output &operator=(const tool_output &);
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

} // namespace silicon::ai::llm
