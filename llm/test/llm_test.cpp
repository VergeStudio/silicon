#include <memory>
#include <silicon/test/test.hpp>
#include <string>
#include <vector>

import silicon.llm;
import silicon.json;

using namespace silicon::llm;

// ── 测试夹具：具体 ITool / IProvider ───────────────────────────

class EchoTool: public ITool {
  public:
    std::string_view name() const override { return "echo"; }
    std::string_view description() const override { return "echoes input"; }
    ToolOutput execute(const ToolCall &call) override {
        ToolOutput out;
        out.content = "echo:" + call.arguments;
        return out;
    }
};

class ConstProvider: public IProvider {
    std::string text_;

  public:
    explicit ConstProvider(std::string t): text_(std::move(t)) {}
    Result<ChatResponse> chat(const Conversation &, const ModelRequestOptions &) override {
        ChatResponse r;
        r.content = text_;
        r.finish_reason = "stop";
        r.prompt_tokens = 3;
        r.completion_tokens = 7;
        return Result<ChatResponse>(std::move(r));
    }
};

// ── ToolRegistry ──────────────────────────────────────────────

TEST_CASE("ToolRegistry 注册并按 name 查询") {
    ToolRegistry reg;
    CHECK(reg.tool_count() == 0);
    CHECK(reg.register_tool(std::make_unique<EchoTool>()));
    CHECK(reg.tool_count() == 1);

    auto *t = reg.get_tool("echo");
    CHECK(t != nullptr);
    CHECK(t->name() == "echo");

    auto out = t->execute(ToolCall{"1", "echo", "\"hi\""});
    CHECK(out.content == "echo:\"hi\"");
}

TEST_CASE("ToolRegistry 重复 name 注册返回 false") {
    ToolRegistry reg;
    CHECK(reg.register_tool(std::make_unique<EchoTool>()));
    CHECK_FALSE(reg.register_tool(std::make_unique<EchoTool>()));
    CHECK(reg.tool_count() == 1);
}

TEST_CASE("ToolRegistry get_tool 未知 name 返回 nullptr") {
    ToolRegistry reg;
    CHECK(reg.get_tool("missing") == nullptr);
}

// ── ProviderRegistry ──────────────────────────────────────────

TEST_CASE("ProviderRegistry 注册/查询/列举") {
    ProviderRegistry reg;
    CHECK(reg.register_provider("openai", std::make_unique<ConstProvider>("a")));
    CHECK(reg.register_provider("anthropic", std::make_unique<ConstProvider>("b")));
    CHECK(reg.list_providers().size() == 2);

    auto *p = reg.get_provider("openai");
    CHECK(p != nullptr);
    auto r = p->chat({}, {});
    CHECK(r);
    CHECK(r->content == "a");

    CHECK(reg.get_provider("missing") == nullptr);
}

TEST_CASE("ProviderRegistry 重复 id 注册返回 false") {
    ProviderRegistry reg;
    CHECK(reg.register_provider("openai", std::make_unique<ConstProvider>("a")));
    CHECK_FALSE(reg.register_provider("openai", std::make_unique<ConstProvider>("b")));
    CHECK(reg.list_providers().size() == 1);
}

// ── ScriptedProvider ──────────────────────────────────────────

TEST_CASE("ScriptedProvider 按 FIFO 返回预置响应") {
    ScriptedProvider p;
    ChatResponse r1;
    r1.content = "first";
    ChatResponse r2;
    r2.content = "second";
    p.enqueue(std::move(r1));
    p.enqueue(std::move(r2));
    CHECK(p.remaining() == 2);

    auto a = p.chat({}, {});
    auto b = p.chat({}, {});
    CHECK(a);
    CHECK(b);
    CHECK(a->content == "first");
    CHECK(b->content == "second");
    CHECK(p.remaining() == 0);
}

TEST_CASE("ScriptedProvider 队列耗尽返回 LLMError") {
    ScriptedProvider p;
    auto r = p.chat({}, {});
    CHECK_FALSE(r);
    CHECK(r.error().message() == "no scripted response");
}

// ── JsonProtocolAdapter ───────────────────────────────────────

TEST_CASE("JsonProtocolAdapter::encode_request 含 model/messages/tools") {
    JsonProtocolAdapter adapter;
    Conversation conv;
    conv.push_back(Message{"system", "be brief"});
    conv.push_back(Message{"user", "hello"});

    ModelRequestOptions opts;
    opts.model = "gpt-4o";
    opts.temperature = 0.2;
    opts.max_tokens = 1024;

    std::vector<std::string> tools = {R"({"name":"echo","description":"e"})"};

    auto json = adapter.encode_request(conv, opts, tools);
    auto parsed = silicon::json::parse(json);
    CHECK(!parsed.is_discarded());
    CHECK(parsed.find("model")->get<std::string>() == "gpt-4o");
    CHECK(parsed.find("temperature")->get<double>() == 0.2);
    CHECK(parsed.find("max_tokens")->get<std::int64_t>() == 1024);
    CHECK(parsed.find("messages")->size() == 2);
    CHECK(parsed.find("tools")->size() == 1);
}

TEST_CASE("JsonProtocolAdapter::decode_response 还原 content/finish_reason/usage") {
    JsonProtocolAdapter adapter;
    std::string raw = R"({
        "choices": [ { "message": { "content": "hi there" }, "finish_reason": "stop" } ],
        "usage": { "prompt_tokens": 11, "completion_tokens": 22 }
    })";

    auto r = adapter.decode_response(raw);
    CHECK(r);
    CHECK(r->content == "hi there");
    CHECK(r->finish_reason == "stop");
    CHECK(r->prompt_tokens == 11);
    CHECK(r->completion_tokens == 22);
}

TEST_CASE("JsonProtocolAdapter::decode_response 非法 JSON 返回 LLMError") {
    JsonProtocolAdapter adapter;
    auto r = adapter.decode_response("{not json");
    CHECK_FALSE(r);
    CHECK(r.error().message() == "invalid json response");
}
