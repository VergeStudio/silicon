module;

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>

module silicon.ai.llm.types;

namespace silicon::ai::llm {

struct chat_response::impl {
  public:
    std::string content_;
    std::string finish_reason_;
    int32_t prompt_tokens_ = 0;
    int32_t completion_tokens_ = 0;
};

chat_response::chat_response(): impl_(std::make_shared<impl>()) {}
chat_response::chat_response(const chat_response &o): impl_(std::make_shared<impl>(*o.impl_)) {}
chat_response &chat_response::operator=(const chat_response &o) {
    if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
    return *this;
}
chat_response::chat_response(chat_response &&) noexcept = default;
chat_response &chat_response::operator=(chat_response &&) noexcept = default;

std::string &chat_response::content() { return impl_->content_; }
const std::string &chat_response::content() const { return impl_->content_; }
std::string &chat_response::finish_reason() { return impl_->finish_reason_; }
const std::string &chat_response::finish_reason() const { return impl_->finish_reason_; }
int32_t &chat_response::prompt_tokens() { return impl_->prompt_tokens_; }
const int32_t &chat_response::prompt_tokens() const { return impl_->prompt_tokens_; }
int32_t &chat_response::completion_tokens() { return impl_->completion_tokens_; }
const int32_t &chat_response::completion_tokens() const { return impl_->completion_tokens_; }

struct message::impl {
  public:
    std::string role_;
    std::string content_;
    std::string tool_call_id_;
};

message::message(): impl_(std::make_shared<impl>()) {}
message::message(std::string r, std::string c, std::string tcid)
    : impl_(std::make_shared<impl>()) {
    impl_->role_ = std::move(r);
    impl_->content_ = std::move(c);
    impl_->tool_call_id_ = std::move(tcid);
}
message::message(const message &o): impl_(std::make_shared<impl>(*o.impl_)) {}
message &message::operator=(const message &o) {
    if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
    return *this;
}
message::message(message &&) noexcept = default;
message &message::operator=(message &&) noexcept = default;

std::string &message::role() { return impl_->role_; }
const std::string &message::role() const { return impl_->role_; }
std::string &message::content() { return impl_->content_; }
const std::string &message::content() const { return impl_->content_; }
std::string &message::tool_call_id() { return impl_->tool_call_id_; }
const std::string &message::tool_call_id() const { return impl_->tool_call_id_; }

struct model_request_options::impl {
  public:
    std::string model_;
    double temperature_ = 0.7;
    int32_t max_tokens_ = 4096;
    std::map<std::string, std::string, std::less<>> extra_;
};

model_request_options::model_request_options(): impl_(std::make_shared<impl>()) {}
model_request_options::model_request_options(const model_request_options &o): impl_(std::make_shared<impl>(*o.impl_)) {}
model_request_options &model_request_options::operator=(const model_request_options &o) {
    if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
    return *this;
}
model_request_options::model_request_options(model_request_options &&) noexcept = default;
model_request_options &model_request_options::operator=(model_request_options &&) noexcept = default;

std::string &model_request_options::model() { return impl_->model_; }
const std::string &model_request_options::model() const { return impl_->model_; }
double &model_request_options::temperature() { return impl_->temperature_; }
const double &model_request_options::temperature() const { return impl_->temperature_; }
int32_t &model_request_options::max_tokens() { return impl_->max_tokens_; }
const int32_t &model_request_options::max_tokens() const { return impl_->max_tokens_; }
std::map<std::string, std::string, std::less<>> &model_request_options::extra() { return impl_->extra_; }
const std::map<std::string, std::string, std::less<>> &model_request_options::extra() const { return impl_->extra_; }

struct tool_call::impl {
  public:
    std::string id_;
    std::string name_;
    std::string arguments_;
};

tool_call::tool_call(): impl_(std::make_shared<impl>()) {}
tool_call::tool_call(std::string i, std::string n, std::string args)
    : impl_(std::make_shared<impl>()) {
    impl_->id_ = std::move(i);
    impl_->name_ = std::move(n);
    impl_->arguments_ = std::move(args);
}
tool_call::tool_call(const tool_call &o): impl_(std::make_shared<impl>(*o.impl_)) {}
tool_call &tool_call::operator=(const tool_call &o) {
    if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
    return *this;
}
tool_call::tool_call(tool_call &&) noexcept = default;
tool_call &tool_call::operator=(tool_call &&) noexcept = default;

std::string &tool_call::id() { return impl_->id_; }
const std::string &tool_call::id() const { return impl_->id_; }
std::string &tool_call::name() { return impl_->name_; }
const std::string &tool_call::name() const { return impl_->name_; }
std::string &tool_call::arguments() { return impl_->arguments_; }
const std::string &tool_call::arguments() const { return impl_->arguments_; }

struct tool_output::impl {
  public:
    std::string content_;
    bool truncated_ = false;
    std::string managed_output_path_;
};

tool_output::tool_output(): impl_(std::make_shared<impl>()) {}
tool_output::tool_output(const tool_output &o): impl_(std::make_shared<impl>(*o.impl_)) {}
tool_output &tool_output::operator=(const tool_output &o) {
    if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
    return *this;
}
tool_output::tool_output(tool_output &&) noexcept = default;
tool_output &tool_output::operator=(tool_output &&) noexcept = default;

std::string &tool_output::content() { return impl_->content_; }
const std::string &tool_output::content() const { return impl_->content_; }
bool &tool_output::truncated() { return impl_->truncated_; }
const bool &tool_output::truncated() const { return impl_->truncated_; }
std::string &tool_output::managed_output_path() { return impl_->managed_output_path_; }
const std::string &tool_output::managed_output_path() const { return impl_->managed_output_path_; }

}
