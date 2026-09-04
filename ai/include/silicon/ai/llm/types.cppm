module;

#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <silicon/ai/common.h>
export module silicon.ai.llm.types;

import silicon.error;

export namespace silicon::ai::llm {

template<typename T>
using result = silicon::error::result<T>;

struct AI_API message {

    struct impl;
    std::shared_ptr<impl> impl_;

  public:
    message();

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

struct AI_API model_request_options {

    struct impl;
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

struct AI_API chat_response {

    struct impl;
    std::shared_ptr<impl> impl_;

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

struct AI_API tool_call {

    struct impl;
    std::shared_ptr<impl> impl_;

  public:
    tool_call();

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

struct AI_API tool_output {

    struct impl;
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

}
