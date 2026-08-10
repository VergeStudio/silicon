#include <memory>
#include <silicon/test/test.hpp>
#include <string>
#include <string_view>
import silicon.plugin;
using namespace silicon::plugin;

namespace {
struct TestPlugin: IPlugin {
    std::string_view Name() const override {
        static auto n = std::string("test");
        return n;
    }
    bool loaded = false;
    bool OnLoad() override {
        loaded = true;
        return true;
    }
};

/// 不继承任何基类，仅具备约定成员 —— 验证 proxy 的非侵入式擦除。
struct DuckPlugin {
    std::string name;
    int load_count = 0;
    int unload_count = 0;

    std::string_view Name() const { return name; }
    bool OnLoad() {
        ++load_count;
        return true;
    }
    bool OnUnload() {
        ++unload_count;
        return true;
    }
    bool OnReload() { return true; }
};
} // namespace

TEST_CASE("PluginRegistry: 注册与查询") {
    PluginRegistry reg;
    auto p = std::make_shared<TestPlugin>();
    CHECK(reg.RegisterPlugin(p));
    CHECK(reg.GetPlugin("test") == p.get());
    CHECK(reg.ListPlugins().size() == 1);
}

TEST_CASE("PluginRegistry: 重复注册失败") {
    PluginRegistry reg;
    auto p1 = std::make_shared<TestPlugin>();
    auto p2 = std::make_shared<TestPlugin>();
    CHECK(reg.RegisterPlugin(p1));
    CHECK_FALSE(reg.RegisterPlugin(p2)); // same name "test"
}

TEST_CASE("PluginRegistry: 移除触发 OnUnload") {
    PluginRegistry reg;
    auto p = std::make_shared<TestPlugin>();
    reg.RegisterPlugin(p);
    CHECK(reg.RemovePlugin("test"));
    CHECK(reg.GetPlugin("test") == nullptr);
}

// ── proxy 类型擦除 ───────────────────────────────────────────────

TEST_CASE("proxy: 非侵入式插件视图（不继承 IPlugin）") {
    DuckPlugin duck{.name = "duck"};
    PluginView v = MakePluginView(duck);
    CHECK(static_cast<bool>(v));
    CHECK(v->Name() == "duck");
    CHECK(v->OnLoad());
    CHECK(duck.load_count == 1);
    CHECK(v->OnReload());
}

TEST_CASE("proxy: 拥有所有权的插件句柄") {
    PluginProxy p = MakePlugin<DuckPlugin>(DuckPlugin{.name = "owned"});
    CHECK(static_cast<bool>(p));
    CHECK(p->Name() == "owned");
    CHECK(p->OnLoad());

    PluginProxy moved = std::move(p);
    CHECK(moved->Name() == "owned");

    PluginProxy empty;
    CHECK_FALSE(static_cast<bool>(empty));
}

TEST_CASE("proxy: 桥接既有 IPlugin 实现") {
    // shared_ptr<T> 本身即 pointer-like，且 TestPlugin 具备全部约定成员，
    // 因此无需任何适配器即可擦除为 PluginProxy。
    auto sp = std::make_shared<TestPlugin>();
    PluginProxy p = sp;
    CHECK(p->Name() == "test");
    CHECK(p->OnLoad());
    CHECK(sp->loaded);
}

TEST_CASE("ProxyPluginRegistry: 注册鸭子类型与查询") {
    ProxyPluginRegistry reg;
    CHECK(reg.Emplace<DuckPlugin>(DuckPlugin{.name = "a"}));
    CHECK(reg.Emplace<DuckPlugin>(DuckPlugin{.name = "b"}));
    CHECK_FALSE(reg.Emplace<DuckPlugin>(DuckPlugin{.name = "a"})); // 重名
    CHECK(reg.List().size() == 2);

    auto *a = reg.Get("a");
    REQUIRE(a != nullptr);
    CHECK((*a)->Name() == "a");
    CHECK(reg.Get("missing") == nullptr);
}

TEST_CASE("ProxyPluginRegistry: 移除触发 OnUnload") {
    ProxyPluginRegistry reg;
    CHECK(reg.Emplace<DuckPlugin>(DuckPlugin{.name = "x"}));
    CHECK(reg.Remove("x"));
    CHECK(reg.Get("x") == nullptr);
    CHECK_FALSE(reg.Remove("x"));
}

TEST_CASE("ProxyPluginRegistry: 混合注册 IPlugin 与鸭子类型") {
    ProxyPluginRegistry reg;
    CHECK(reg.Register(std::make_shared<TestPlugin>()));   // 继承体系
    CHECK(reg.Emplace<DuckPlugin>(DuckPlugin{.name = "d"})); // 非侵入式
    CHECK(reg.List().size() == 2);
    CHECK(reg.Get("test") != nullptr);
    CHECK(reg.Get("d") != nullptr);
}
