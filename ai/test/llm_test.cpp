#include <memory>
#include <silicon/test/test.h>
#include <string>
#include <vector>

import silicon.ai.llm;
import silicon.json;
import silicon.proxy;

using namespace silicon::ai::llm;

class echo_tool {
  public:
    std::string_view name() const { return "echo"; }
    std::string_view description() const { return "echoes input"; }
    tool_output execute(const tool_call &call) {
        tool_output out;
        out.content() = "echo:" + call.arguments();
        return out;
    }
};

class const_provider {
    std::string text_;

  public:
    explicit const_provider(std::string t): text_(std::move(t)) {}
    silicon::error::result<chat_response> chat(const conversation &, const model_request_options &) {
        chat_response r;
        r.content() = text_;
        r.finish_reason() = "stop";
        r.prompt_tokens() = 3;
        r.completion_tokens() = 7;
        return silicon::error::result<chat_response>(std::move(r));
    }
};

TEST_CASE("tool_registry 注册并按 name 查询") {
    tool_registry reg;
    CHECK(reg.tool_count() == 0);
    CHECK(reg.register_tool(make_tool<echo_tool>()));
    CHECK(reg.tool_count() == 1);

    auto t = reg.get_tool("echo");
    CHECK(t);
    CHECK(t->name() == "echo");

    auto out = t->execute(tool_call{"1", "echo", "\"hi\""});
    CHECK(out.content() == "echo:\"hi\"");
}

TEST_CASE("tool_registry 重复 name 注册返回 false") {
    tool_registry reg;
    CHECK(reg.register_tool(make_tool<echo_tool>()));
    CHECK_FALSE(reg.register_tool(make_tool<echo_tool>()));
    CHECK(reg.tool_count() == 1);
}

TEST_CASE("tool_registry get_tool 未知 name 返回空句柄") {
    tool_registry reg;
    CHECK_FALSE(reg.get_tool("missing"));
}

TEST_CASE("provider_registry 注册/查询/列举") {
    provider_registry reg;
    CHECK(reg.register_provider("openai", make_provider<const_provider>("a")));
    CHECK(reg.register_provider("anthropic", make_provider<const_provider>("b")));
    CHECK(reg.list_providers().size() == 2);

    auto p = reg.get_provider("openai");
    CHECK(p);
    auto r = p->chat({}, {});
    CHECK(r);
    CHECK(r->content() == "a");

    CHECK_FALSE(reg.get_provider("missing"));
}

TEST_CASE("provider_registry 重复 id 注册返回 false") {
    provider_registry reg;
    CHECK(reg.register_provider("openai", make_provider<const_provider>("a")));
    CHECK_FALSE(reg.register_provider("openai", make_provider<const_provider>("b")));
    CHECK(reg.list_providers().size() == 1);
}

TEST_CASE("scripted_provider 按 FIFO 返回预置响应") {
    scripted_provider p;
    chat_response r1;
    r1.content() = "first";
    chat_response r2;
    r2.content() = "second";
    p.enqueue(std::move(r1));
    p.enqueue(std::move(r2));
    CHECK(p.remaining() == 2);

    auto a = p.chat({}, {});
    auto b = p.chat({}, {});
    CHECK(a);
    CHECK(b);
    CHECK(a->content() == "first");
    CHECK(b->content() == "second");
    CHECK(p.remaining() == 0);
}

TEST_CASE("scripted_provider 队列耗尽返回 llm_error") {
    scripted_provider p;
    auto r = p.chat({}, {});
    CHECK_FALSE(r);
    CHECK(r.error().message() == "llm provider unavailable");
}

TEST_CASE("json_protocol_adapter::encode_request 含 model/messages/tools") {
    json_protocol_adapter adapter;
    conversation conv;
    conv.push_back(message{"system", "be brief"});
    conv.push_back(message{"user", "hello"});

    model_request_options opts;
    opts.model() = "gpt-4o";
    opts.temperature() = 0.2;
    opts.max_tokens() = 1024;

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

TEST_CASE("json_protocol_adapter::decode_response 还原 content/finish_reason/usage") {
    json_protocol_adapter adapter;
    std::string raw = R"({
        "choices": [ { "message": { "content": "hi there" }, "finish_reason": "stop" } ],
        "usage": { "prompt_tokens": 11, "completion_tokens": 22 }
    })";

    auto r = adapter.decode_response(raw);
    CHECK(r);
    CHECK(r->content() == "hi there");
    CHECK(r->finish_reason() == "stop");
    CHECK(r->prompt_tokens() == 11);
    CHECK(r->completion_tokens() == 22);
}

TEST_CASE("json_protocol_adapter::decode_response 非法 JSON 返回 llm_error") {
    json_protocol_adapter adapter;
    auto r = adapter.decode_response("{not json");
    CHECK_FALSE(r);
    CHECK(r.error().message() == "invalid llm response");
}

namespace {
    struct di_probe_category : std::error_category {
        const char *name() const noexcept override { return "di-probe"; }
        std::string message(int) const override { return "probe"; }
    };
}

TEST_CASE("DI: 模块统一引用组合根注入的 category 实例") {
    di_probe_category probe;
    inject_llm_error_category(probe);

    scripted_provider p;
    auto r = p.chat({}, {});
    REQUIRE_FALSE(r);
    CHECK(std::string(r.error().category().name()) == "di-probe");
}
