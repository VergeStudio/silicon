module;

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <expected>

module silicon.ai.llm;
import silicon.ai.llm.error;

import silicon.json;
import silicon.core;

namespace silicon::ai::llm {

// ── chat_response PIMPL（impl 完整定义，接口单元仅前向声明） ────────

struct chat_response::impl {
  public:
    std::string content_;
    std::string finish_reason_; // "stop" / "length" / "tool_calls"
    int32_t prompt_tokens_ = 0;
    int32_t completion_tokens_ = 0;
};

chat_response::chat_response() : impl_(std::make_shared<impl>()) {}
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

// ── message PIMPL（impl 完整定义，接口单元仅前向声明） ────────────

struct message::impl {
  public:
    std::string role_; // "user" / "assistant" / "system" / "tool"
    std::string content_;
    std::string tool_call_id_;
};

message::message() : impl_(std::make_shared<impl>()) {}
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

// ── model_request_options PIMPL ─────────────────────────────────

struct model_request_options::impl {
  public:
    std::string model_;
    double temperature_ = 0.7;
    int32_t max_tokens_ = 4096;
    std::map<std::string, std::string, std::less<>> extra_;
};

model_request_options::model_request_options() : impl_(std::make_shared<impl>()) {}
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

// ── tool_call PIMPL ──────────────────────────────────────────────

struct tool_call::impl {
  public:
    std::string id_;
    std::string name_;
    std::string arguments_; // JSON string
};

tool_call::tool_call() : impl_(std::make_shared<impl>()) {}
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

// ── tool_output PIMPL ───────────────────────────────────────────

struct tool_output::impl {
  public:
    std::string content_;
    bool truncated_ = false;
    std::string managed_output_path_;
};

tool_output::tool_output() : impl_(std::make_shared<impl>()) {}
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

// ── http_provider::http_result PIMPL ────────────────────────────

struct http_provider::http_result::impl {
  public:
    int status_ = 0;
    std::string body_;
};

http_provider::http_result::http_result() : impl_(std::make_shared<impl>()) {}
http_provider::http_result::http_result(const http_result &o): impl_(std::make_shared<impl>(*o.impl_)) {}
http_provider::http_result &http_provider::http_result::operator=(const http_result &o) {
    if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
    return *this;
}
http_provider::http_result::http_result(http_result &&) noexcept = default;
http_provider::http_result &http_provider::http_result::operator=(http_result &&) noexcept = default;

int &http_provider::http_result::status() { return impl_->status_; }
const int &http_provider::http_result::status() const { return impl_->status_; }
std::string &http_provider::http_result::body() { return impl_->body_; }
const std::string &http_provider::http_result::body() const { return impl_->body_; }

tool_registry::tool_registry() : impl_(std::make_unique<impl>()) {}
provider_registry::provider_registry() : impl_(std::make_unique<impl>()) {}
scripted_provider::scripted_provider() : impl_(std::make_unique<impl>()) {}

bool tool_registry::register_tool(std::unique_ptr<i_tool> tool) {
    auto name = std::string(tool->name());
    return impl_->tools_.emplace(std::move(name), std::move(tool)).second;
}

i_tool *tool_registry::get_tool(std::string_view name) const {
    auto it = impl_->tools_.find(name);
    return it != impl_->tools_.end() ? it->second.get() : nullptr;
}

std::size_t tool_registry::tool_count() const { return impl_->tools_.size(); }

bool provider_registry::register_provider(std::string id, std::unique_ptr<i_provider> provider) {
    return impl_->providers_.emplace(std::move(id), std::move(provider)).second;
}

i_provider *provider_registry::get_provider(std::string_view id) const {
    auto it = impl_->providers_.find(id);
    return it != impl_->providers_.end() ? it->second.get() : nullptr;
}

std::vector<std::string> provider_registry::list_providers() const {
    std::vector<std::string> ids;
    for(const auto &[k, v]: impl_->providers_) ids.push_back(k);
    return ids;
}

std::string json_protocol_adapter::encode_request(
        const conversation &conv,
        const model_request_options &opts,
        const std::vector<std::string> &tool_defs
) const {
    using namespace silicon::json;
    JsonValue req = JsonValue::object();
    req["model"] = JsonValue(opts.model());
    req["temperature"] = JsonValue(opts.temperature());
    req["max_tokens"] =
            JsonValue(static_cast<std::int64_t>(opts.max_tokens()));

    JsonValue messages = JsonValue::array();
    for(const auto &m: conv) {
        JsonValue msg = JsonValue::object();
        msg["role"] = JsonValue(m.role());
        msg["content"] = JsonValue(m.content());
        if(!m.tool_call_id().empty()) msg["tool_call_id"] = JsonValue(m.tool_call_id());
        messages.push_back(std::move(msg));
    }
    req["messages"] = std::move(messages);

    if(!tool_defs.empty()) {
        JsonValue tools = JsonValue::array();
        for(const auto &td: tool_defs) {
            auto parsed = parse(td);
            if(!parsed.is_discarded()) tools.push_back(parsed);
        }
        req["tools"] = std::move(tools);
    }
    return req.dump();
}

result<chat_response> json_protocol_adapter::decode_response(std::string_view raw) const {
    using namespace silicon::json;
    auto v = parse(raw);
    if(v.is_discarded()) return std::unexpected(make_error_code(llm_error::kInvalidResponse));
    if(!v.is_object())
        return std::unexpected(make_error_code(llm_error::kInvalidResponse));

    chat_response resp;

    if(auto c = v.find("choices");
       c != v.end() && c->is_array()) {
        const auto &arr = *c;
        if(!arr.empty() && arr[0].is_object()) {
            const auto &choice = arr[0];
            if(auto m = choice.find("message");
               m != choice.end() && m->is_object()) {
                const auto &msg = *m;
                if(auto cc = msg.find("content");
                   cc != msg.end() && cc->is_string())
                    resp.content() = cc->get<std::string>();
            }
            if(auto fr = choice.find("finish_reason");
               fr != choice.end() && fr->is_string())
                resp.finish_reason() = fr->get<std::string>();
        }
    }
    if(auto u = v.find("usage");
       u != v.end() && u->is_object()) {
        if(auto pt = u->find("prompt_tokens");
           pt != u.end() && pt->is_number_integer())
            resp.prompt_tokens() = static_cast<int32_t>((*pt).get<std::int64_t>());
        if(auto ct = u->find("completion_tokens");
           ct != u.end() && ct->is_number_integer())
            resp.completion_tokens() = static_cast<int32_t>((*ct).get<std::int64_t>());
    }
    return result<chat_response>(std::move(resp));
}

void scripted_provider::enqueue(chat_response r) { impl_->queue_.push(std::move(r)); }

std::size_t scripted_provider::remaining() const { return impl_->queue_.size(); }

result<chat_response> scripted_provider::chat(const conversation &, const model_request_options &) {
    if(impl_->queue_.empty())
        return std::unexpected(make_error_code(llm_error::kProviderUnavailable));
    chat_response r = std::move(impl_->queue_.front());
    impl_->queue_.pop();
    return result<chat_response>(std::move(r));
}

std::string http_provider::env_or(const char *name, std::string def) {
    std::string v = silicon::os::get_env(name);
    return v.empty() ? def : v;
}

http_provider::http_result http_provider::post_json(const std::string &url, const std::string &body) const {
    namespace fs = std::filesystem;
    fs::path tmp = fs::temp_directory_path() /
                   ("sb_req_" + std::to_string(static_cast<long long>(std::time(nullptr))) + ".json");
    {
        std::ofstream f(tmp, std::ios::binary);
        if(!f) return {};
        f << body;
    }

    std::string cmd = "curl -s -m 60 -X POST";
    cmd += " -H \"content-Type: application/json\"";
    if(!impl_->api_key_.empty())
        cmd += " -H \"Authorization: Bearer " + impl_->api_key_ + "\"";
    cmd += " -d @\"" + tmp.string() + "\"";
    cmd += " -w \"\\n%{http_code}\"";
    cmd += " \"" + url + "\"";

#if defined(SILICON_PLATFORM_WINDOWS)
    FILE *pipe = _popen(cmd.c_str(), "r");
#else
    FILE *pipe = popen(cmd.c_str(), "r");
#endif
    http_result r;
    if(pipe) {
        std::string all;
        char buf[4096];
        while(std::fgets(buf, sizeof(buf), pipe)) all += buf;
#if defined(SILICON_PLATFORM_WINDOWS)
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        auto nl = all.rfind('\n');
        if(nl != std::string::npos && nl + 1 < all.size()) {
            r.body() = all.substr(0, nl);
            r.status() = std::atoi(all.substr(nl + 1).c_str());
        } else {
            r.body() = std::move(all);
        }
    }
    std::error_code ec;
    fs::remove(tmp, ec);
    return r;
}

http_provider::http_provider() {
    impl_->base_url_ = env_or("SILICONBUDDY_LLM_BASE_URL", "https://api.openai.com/v1");
    impl_->api_key_ = env_or("SILICONBUDDY_LLM_API_KEY", "");
    impl_->model_ = env_or("SILICONBUDDY_LLM_MODEL", "gpt-4o-mini");
}

bool http_provider::configured() const { return !impl_->api_key_.empty(); }

std::string_view http_provider::model_name() const { return impl_->model_; }

result<chat_response> http_provider::chat(const conversation &conv, const model_request_options &opts) {
    model_request_options o = opts;
    if(o.model().empty()) o.model() = impl_->model_;

    std::string body = impl_->adapter_.encode_request(conv, o, {});
    http_result r = post_json(impl_->base_url_ + "/chat/completions", body);
    if(r.status() != 200) {
        return std::unexpected(make_error_code(llm_error::kProviderUnavailable));
    }
    return impl_->adapter_.decode_response(r.body());
}


} // namespace silicon::ai::llm
