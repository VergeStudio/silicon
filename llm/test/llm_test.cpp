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
    std::string_view Name() const override { return "echo"; }
    std::string_view Description() const override { return "echoes input"; }
    ToolOutput Execute(const ToolCall &call) override {
        ToolOutput out;
        out.Content() = "echo:" + call.Arguments();
        return out;
    }
};

class ConstProvider: public IProvider {
    std::string text_;

  public:
    explicit ConstProvider(std::string t): text_(std::move(t)) {}
    Result<ChatResponse> Chat(const Conversation &, const ModelRequestOptions &) override {
        ChatResponse r;
        r.Content() = text_;
        r.FinishReason() = "stop";
        r.PromptTokens() = 3;
        r.CompletionTokens() = 7;
        return Result<ChatResponse>(std::move(r));
    }
};

// ── IToolRegistry ──────────────────────────────────────────────

TEST_CASE("IToolRegistry 注册并按 name 查询") {
    ToolRegistry reg;
    CHECK(reg.ToolCount() == 0);
    CHECK(reg.RegisterTool(std::make_unique<EchoTool>()));
    CHECK(reg.ToolCount() == 1);

    auto *t = reg.GetTool("echo");
    CHECK(t != nullptr);
    CHECK(t->Name() == "echo");

    auto out = t->Execute(ToolCall{"1", "echo", "\"hi\""});
    CHECK(out.Content() == "echo:\"hi\"");
}

TEST_CASE("IToolRegistry 重复 name 注册返回 false") {
    ToolRegistry reg;
    CHECK(reg.RegisterTool(std::make_unique<EchoTool>()));
    CHECK_FALSE(reg.RegisterTool(std::make_unique<EchoTool>()));
    CHECK(reg.ToolCount() == 1);
}

TEST_CASE("IToolRegistry get_tool 未知 name 返回 nullptr") {
    ToolRegistry reg;
    CHECK(reg.GetTool("missing") == nullptr);
}

// ── IProviderRegistry ──────────────────────────────────────────

TEST_CASE("IProviderRegistry 注册/查询/列举") {
    ProviderRegistry reg;
    CHECK(reg.RegisterProvider("openai", std::make_unique<ConstProvider>("a")));
    CHECK(reg.RegisterProvider("anthropic", std::make_unique<ConstProvider>("b")));
    CHECK(reg.ListProviders().size() == 2);

    auto *p = reg.GetProvider("openai");
    CHECK(p != nullptr);
    auto r = p->Chat({}, {});
    CHECK(r);
    CHECK(r->Content() == "a");

    CHECK(reg.GetProvider("missing") == nullptr);
}

TEST_CASE("IProviderRegistry 重复 id 注册返回 false") {
    ProviderRegistry reg;
    CHECK(reg.RegisterProvider("openai", std::make_unique<ConstProvider>("a")));
    CHECK_FALSE(reg.RegisterProvider("openai", std::make_unique<ConstProvider>("b")));
    CHECK(reg.ListProviders().size() == 1);
}

// ── ScriptedProvider ──────────────────────────────────────────

TEST_CASE("ScriptedProvider 按 FIFO 返回预置响应") {
    ScriptedProvider p;
    ChatResponse r1;
    r1.Content() = "first";
    ChatResponse r2;
    r2.Content() = "second";
    p.Enqueue(std::move(r1));
    p.Enqueue(std::move(r2));
    CHECK(p.Remaining() == 2);

    auto a = p.Chat({}, {});
    auto b = p.Chat({}, {});
    CHECK(a);
    CHECK(b);
    CHECK(a->Content() == "first");
    CHECK(b->Content() == "second");
    CHECK(p.Remaining() == 0);
}

TEST_CASE("ScriptedProvider 队列耗尽返回 LLMError") {
    ScriptedProvider p;
    auto r = p.Chat({}, {});
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
    opts.Model() = "gpt-4o";
    opts.Temperature() = 0.2;
    opts.MaxTokens() = 1024;

    std::vector<std::string> tools = {R"({"name":"echo","description":"e"})"};

    auto json = adapter.EncodeRequest(conv, opts, tools);
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

    auto r = adapter.DecodeResponse(raw);
    CHECK(r);
    CHECK(r->Content() == "hi there");
    CHECK(r->FinishReason() == "stop");
    CHECK(r->PromptTokens() == 11);
    CHECK(r->CompletionTokens() == 22);
}

TEST_CASE("JsonProtocolAdapter::decode_response 非法 JSON 返回 LLMError") {
    JsonProtocolAdapter adapter;
    auto r = adapter.DecodeResponse("{not json");
    CHECK_FALSE(r);
    CHECK(r.error().message() == "invalid json response");
}
