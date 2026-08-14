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

bool tool_registry::register_tool(tool_proxy tool) {
    auto name = std::string(tool->name());
    return impl_->tools_.emplace(std::move(name), std::move(tool)).second;
}

tool_proxy tool_registry::get_tool(std::string_view name) const {
    auto it = impl_->tools_.find(name);
    return it != impl_->tools_.end() ? it->second : tool_proxy{};
}

std::size_t tool_registry::tool_count() const { return impl_->tools_.size(); }

bool provider_registry::register_provider(std::string id, provider_proxy provider) {
    return impl_->providers_.emplace(std::move(id), std::move(provider)).second;
}

provider_proxy provider_registry::get_provider(std::string_view id) const {
    auto it = impl_->providers_.find(id);
    return it != impl_->providers_.end() ? it->second : provider_proxy{};
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
