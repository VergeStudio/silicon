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

ToolRegistry::ToolRegistry() : m_p(std::make_unique<P>()) {}
ProviderRegistry::ProviderRegistry() : m_p(std::make_unique<P>()) {}
ScriptedProvider::ScriptedProvider() : m_p(std::make_unique<P>()) {}

bool ToolRegistry::register_tool(std::unique_ptr<ITool> tool) {
    auto name = std::string(tool->name());
    return m_p->tools_.emplace(std::move(name), std::move(tool)).second;
}

ITool *ToolRegistry::get_tool(std::string_view name) const {
    auto it = m_p->tools_.find(name);
    return it != m_p->tools_.end() ? it->second.get() : nullptr;
}

std::size_t ToolRegistry::tool_count() const { return m_p->tools_.size(); }

bool ProviderRegistry::register_provider(std::string id, std::unique_ptr<IProvider> provider) {
    return m_p->providers_.emplace(std::move(id), std::move(provider)).second;
}

IProvider *ProviderRegistry::get_provider(std::string_view id) const {
    auto it = m_p->providers_.find(id);
    return it != m_p->providers_.end() ? it->second.get() : nullptr;
}

std::vector<std::string> ProviderRegistry::list_providers() const {
    std::vector<std::string> ids;
    for(const auto &[k, v]: m_p->providers_) ids.push_back(k);
    return ids;
}

std::string JsonProtocolAdapter::encode_request(
        const Conversation &conv,
        const ModelRequestOptions &opts,
        const std::vector<std::string> &tool_defs
) const {
    using namespace silicon::json;
    JsonValue req = JsonValue::object();
    req["model"] = JsonValue(opts.model);
    req["temperature"] = JsonValue(opts.temperature);
    req["max_tokens"] =
            JsonValue(static_cast<std::int64_t>(opts.max_tokens));

    JsonValue messages = JsonValue::array();
    for(const auto &m: conv) {
        JsonValue msg = JsonValue::object();
        msg["role"] = JsonValue(m.role);
        msg["content"] = JsonValue(m.content);
        if(!m.tool_call_id.empty()) msg["tool_call_id"] = JsonValue(m.tool_call_id);
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

Result<ChatResponse> JsonProtocolAdapter::decode_response(std::string_view raw) const {
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
                    resp.content = cc->get<std::string>();
            }
            if(auto fr = choice.find("finish_reason");
               fr != choice.end() && fr->is_string())
                resp.finish_reason = fr->get<std::string>();
        }
    }
    if(auto u = v.find("usage");
       u != v.end() && u->is_object()) {
        if(auto pt = u->find("prompt_tokens");
           pt != u->end() && pt->is_number_integer())
            resp.prompt_tokens = static_cast<int32_t>((*pt).get<std::int64_t>());
        if(auto ct = u->find("completion_tokens");
           ct != u->end() && ct->is_number_integer())
            resp.completion_tokens = static_cast<int32_t>((*ct).get<std::int64_t>());
    }
    return Result<ChatResponse>(std::move(resp));
}

void ScriptedProvider::enqueue(ChatResponse r) { m_p->queue_.push(std::move(r)); }

std::size_t ScriptedProvider::remaining() const { return m_p->queue_.size(); }

Result<ChatResponse> ScriptedProvider::chat(const Conversation &, const ModelRequestOptions &) {
    if(m_p->queue_.empty())
        return Result<ChatResponse>(silicon::exception::LLMError{"no scripted response"});
    ChatResponse r = std::move(m_p->queue_.front());
    m_p->queue_.pop();
    return Result<ChatResponse>(std::move(r));
}

std::string HttpProvider::env_or(const char *name, std::string def) {
    std::string v = silicon::os::get_env(name);
    return v.empty() ? def : v;
}

HttpProvider::HttpResult HttpProvider::post_json(const std::string &url, const std::string &body) const {
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
    if(!api_key_.empty())
        cmd += " -H \"Authorization: Bearer " + api_key_ + "\"";
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
            r.body = all.substr(0, nl);
            r.status = std::atoi(all.substr(nl + 1).c_str());
        } else {
            r.body = std::move(all);
        }
    }
    std::error_code ec;
    fs::remove(tmp, ec);
    return r;
}

HttpProvider::HttpProvider()
    : base_url_(env_or("SILICONBUDDY_LLM_BASE_URL", "https://api.openai.com/v1")),
      api_key_(env_or("SILICONBUDDY_LLM_API_KEY", "")),
      model_(env_or("SILICONBUDDY_LLM_MODEL", "gpt-4o-mini")) {}

bool HttpProvider::configured() const { return !api_key_.empty(); }

std::string_view HttpProvider::model_name() const { return model_; }

Result<ChatResponse> HttpProvider::chat(const Conversation &conv, const ModelRequestOptions &opts) {
    ModelRequestOptions o = opts;
    if(o.model.empty()) o.model = model_;

    std::string body = adapter_.encode_request(conv, o, {});
    HttpResult r = post_json(base_url_ + "/chat/completions", body);
    if(r.status != 200) {
        return Result<ChatResponse>(silicon::exception::LLMError{
                "LLM HTTP " + std::to_string(r.status) + ": " + r.body
        });
    }
    return adapter_.decode_response(r.body);
}

} // namespace silicon::llm
