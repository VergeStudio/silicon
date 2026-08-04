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

module silicon.llm;

import silicon.json;
import silicon.core;
import silicon.exception;

namespace silicon::llm {

DefaultToolRegistry::DefaultToolRegistry() : impl_(std::make_unique<Impl>()) {}
DefaultProviderRegistry::DefaultProviderRegistry() : impl_(std::make_unique<Impl>()) {}
ScriptedProvider::ScriptedProvider() : impl_(std::make_unique<Impl>()) {}

bool DefaultToolRegistry::RegisterTool(std::unique_ptr<Tool> tool) {
    auto name = std::string(tool->Name());
    return impl_->tools_.emplace(std::move(name), std::move(tool)).second;
}

Tool *DefaultToolRegistry::GetTool(std::string_view name) const {
    auto it = impl_->tools_.find(name);
    return it != impl_->tools_.end() ? it->second.get() : nullptr;
}

std::size_t DefaultToolRegistry::ToolCount() const { return impl_->tools_.size(); }

bool DefaultProviderRegistry::RegisterProvider(std::string id, std::unique_ptr<Provider> provider) {
    return impl_->providers_.emplace(std::move(id), std::move(provider)).second;
}

Provider *DefaultProviderRegistry::GetProvider(std::string_view id) const {
    auto it = impl_->providers_.find(id);
    return it != impl_->providers_.end() ? it->second.get() : nullptr;
}

std::vector<std::string> DefaultProviderRegistry::ListProviders() const {
    std::vector<std::string> ids;
    for(const auto &[k, v]: impl_->providers_) ids.push_back(k);
    return ids;
}

std::string JsonProtocolAdapter::EncodeRequest(
        const Conversation &conv,
        const ModelRequestOptions &opts,
        const std::vector<std::string> &tool_defs
) const {
    using namespace silicon::json;
    JsonValue req = JsonValue::object();
    req["model"] = JsonValue(opts.Model());
    req["temperature"] = JsonValue(opts.Temperature());
    req["max_tokens"] =
            JsonValue(static_cast<std::int64_t>(opts.MaxTokens()));

    JsonValue messages = JsonValue::array();
    for(const auto &m: conv) {
        JsonValue msg = JsonValue::object();
        msg["role"] = JsonValue(m.Role());
        msg["content"] = JsonValue(m.Content());
        if(!m.ToolCallId().empty()) msg["tool_call_id"] = JsonValue(m.ToolCallId());
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

Result<ChatResponse> JsonProtocolAdapter::DecodeResponse(std::string_view raw) const {
    using namespace silicon::json;
    auto v = parse(raw);
    if(v.is_discarded()) return Result<ChatResponse>(silicon::exception::LLMError{"invalid json response"});
    if(!v.is_object())
        return Result<ChatResponse>(silicon::exception::LLMError{"response is not an object"});

    ChatResponse resp;

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
                    resp.Content() = cc->get<std::string>();
            }
            if(auto fr = choice.find("finish_reason");
               fr != choice.end() && fr->is_string())
                resp.FinishReason() = fr->get<std::string>();
        }
    }
    if(auto u = v.find("usage");
       u != v.end() && u->is_object()) {
        if(auto pt = u->find("prompt_tokens");
           pt != u->end() && pt->is_number_integer())
            resp.PromptTokens() = static_cast<int32_t>((*pt).get<std::int64_t>());
        if(auto ct = u->find("completion_tokens");
           ct != u->end() && ct->is_number_integer())
            resp.CompletionTokens() = static_cast<int32_t>((*ct).get<std::int64_t>());
    }
    return Result<ChatResponse>(std::move(resp));
}

void ScriptedProvider::Enqueue(ChatResponse r) { impl_->queue_.push(std::move(r)); }

std::size_t ScriptedProvider::Remaining() const { return impl_->queue_.size(); }

Result<ChatResponse> ScriptedProvider::Chat(const Conversation &, const ModelRequestOptions &) {
    if(impl_->queue_.empty())
        return Result<ChatResponse>(silicon::exception::LLMError{"no scripted response"});
    ChatResponse r = std::move(impl_->queue_.front());
    impl_->queue_.pop();
    return Result<ChatResponse>(std::move(r));
}

std::string HttpProvider::EnvOr(const char *name, std::string def) {
    std::string v = silicon::os::GetEnv(name);
    return v.empty() ? def : v;
}

HttpProvider::HttpResult HttpProvider::PostJson(const std::string &url, const std::string &body) const {
    namespace fs = std::filesystem;
    fs::path tmp = fs::temp_directory_path() /
                   ("sb_req_" + std::to_string(static_cast<long long>(std::time(nullptr))) + ".json");
    {
        std::ofstream f(tmp, std::ios::binary);
        if(!f) return {};
        f << body;
    }

    std::string cmd = "curl -s -m 60 -X POST";
    cmd += " -H \"Content-Type: application/json\"";
    if(!impl_->api_key_.empty())
        cmd += " -H \"Authorization: Bearer " + impl_->api_key_ + "\"";
    cmd += " -d @\"" + tmp.string() + "\"";
    cmd += " -w \"\\n%{http_code}\"";
    cmd += " \"" + url + "\"";

#ifdef _WIN32
    FILE *pipe = _popen(cmd.c_str(), "r");
#else
    FILE *pipe = popen(cmd.c_str(), "r");
#endif
    HttpResult r;
    if(pipe) {
        std::string all;
        char buf[4096];
        while(std::fgets(buf, sizeof(buf), pipe)) all += buf;
#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        auto nl = all.rfind('\n');
        if(nl != std::string::npos && nl + 1 < all.size()) {
            r.Body() = all.substr(0, nl);
            r.Status() = std::atoi(all.substr(nl + 1).c_str());
        } else {
            r.Body() = std::move(all);
        }
    }
    std::error_code ec;
    fs::remove(tmp, ec);
    return r;
}

HttpProvider::HttpProvider() {
    impl_->base_url_ = EnvOr("SILICONBUDDY_LLM_BASE_URL", "https://api.openai.com/v1");
    impl_->api_key_ = EnvOr("SILICONBUDDY_LLM_API_KEY", "");
    impl_->model_ = EnvOr("SILICONBUDDY_LLM_MODEL", "gpt-4o-mini");
}

bool HttpProvider::Configured() const { return !impl_->api_key_.empty(); }

std::string_view HttpProvider::ModelName() const { return impl_->model_; }

Result<ChatResponse> HttpProvider::Chat(const Conversation &conv, const ModelRequestOptions &opts) {
    ModelRequestOptions o = opts;
    if(o.Model().empty()) o.Model() = impl_->model_;

    std::string body = impl_->adapter_.EncodeRequest(conv, o, {});
    HttpResult r = PostJson(impl_->base_url_ + "/chat/completions", body);
    if(r.Status() != 200) {
        return Result<ChatResponse>(silicon::exception::LLMError{
                "LLM HTTP " + std::to_string(r.Status()) + ": " + r.Body()
        });
    }
    return impl_->adapter_.DecodeResponse(r.Body());
}

} // namespace silicon::llm
