#include <memory>
#include <silicon/test/test.hpp>
#include <string>
#include <string_view>
import silicon.plugin;
using namespace silicon::plugin;

namespace {
struct TestPlugin: i_plugin {
    std::string_view name() const override {
        static auto n = std::string("test");
        return n;
    }
    bool loaded = false;
    bool on_load() override {
        loaded = true;
        return true;
    }
};

/// 不继承任何基类，仅具备约定成员 —— 验证 proxy 的非侵入式擦除。
struct DuckPlugin {
    std::string name;
    int load_count = 0;
    int unload_count = 0;

    std::string_view name() const { return name; }
    bool on_load() {
        ++load_count;
        return true;
    }
    bool on_unload() {
        ++unload_count;
        return true;
    }
    bool on_reload() { return true; }
};
} // namespace

TEST_CASE("plugin_registry: 注册与查询") {
    plugin_registry reg;
    auto p = std::make_shared<TestPlugin>();
    CHECK(reg.register_plugin(p).has_value());
    CHECK(reg.get_plugin("test") == p.get());
    CHECK(reg.list_plugins().size() == 1);
}

TEST_CASE("plugin_registry: 重复注册失败") {
    plugin_registry reg;
    auto p1 = std::make_shared<TestPlugin>();
    auto p2 = std::make_shared<TestPlugin>();
    CHECK(reg.register_plugin(p1).has_value());
    CHECK_FALSE(reg.register_plugin(p2).has_value()); // same name "test"
}

TEST_CASE("plugin_registry: 移除触发 on_unload") {
    plugin_registry reg;
    auto p = std::make_shared<TestPlugin>();
    CHECK(reg.register_plugin(p).has_value());
    CHECK(reg.remove_plugin("test").has_value());
    CHECK(reg.get_plugin("test") == nullptr);
}

// ── proxy 类型擦除 ───────────────────────────────────────────────

TEST_CASE("proxy: 非侵入式插件视图（不继承 i_plugin）") {
    DuckPlugin duck{.name = "duck"};
    plugin_view v = make_plugin_view(duck);
    CHECK(static_cast<bool>(v));
    CHECK(v->name() == "duck");
    CHECK(v->on_load());
    CHECK(duck.load_count == 1);
    CHECK(v->on_reload());
}

TEST_CASE("proxy: 拥有所有权的插件句柄") {
    plugin_proxy p = make_plugin<DuckPlugin>(DuckPlugin{.name = "owned"});
    CHECK(static_cast<bool>(p));
    CHECK(p->name() == "owned");
    CHECK(p->on_load());

    plugin_proxy moved = std::move(p);
    CHECK(moved->name() == "owned");

    plugin_proxy empty;
    CHECK_FALSE(static_cast<bool>(empty));
}

TEST_CASE("proxy: 桥接既有 i_plugin 实现") {
    // shared_ptr<T> 本身即 pointer-like，且 TestPlugin 具备全部约定成员，
    // 因此无需任何适配器即可擦除为 plugin_proxy。
    auto sp = std::make_shared<TestPlugin>();
    plugin_proxy p = sp;
    CHECK(p->name() == "test");
    CHECK(p->on_load());
    CHECK(sp->loaded);
}

TEST_CASE("proxy_plugin_registry: 注册鸭子类型与查询") {
    proxy_plugin_registry reg;
    CHECK(reg.emplace<DuckPlugin>(DuckPlugin{.name = "a"}).has_value());
    CHECK(reg.emplace<DuckPlugin>(DuckPlugin{.name = "b"}).has_value());
    CHECK_FALSE(reg.emplace<DuckPlugin>(DuckPlugin{.name = "a"}).has_value()); // 重名
    CHECK(reg.list().size() == 2);

    auto *a = reg.get("a");
    REQUIRE(a != nullptr);
    CHECK((*a)->name() == "a");
    CHECK(reg.get("missing") == nullptr);
}

TEST_CASE("proxy_plugin_registry: 移除触发 on_unload") {
    proxy_plugin_registry reg;
    CHECK(reg.emplace<DuckPlugin>(DuckPlugin{.name = "x"}).has_value());
    CHECK(reg.remove("x").has_value());
    CHECK(reg.get("x") == nullptr);
    CHECK_FALSE(reg.remove("x").has_value());
}

TEST_CASE("proxy_plugin_registry: 混合注册 i_plugin 与鸭子类型") {
    proxy_plugin_registry reg;
    CHECK(reg.register_plugin(std::make_shared<TestPlugin>()).has_value());   // 继承体系
    CHECK(reg.emplace<DuckPlugin>(DuckPlugin{.name = "d"}).has_value()); // 非侵入式
    CHECK(reg.list().size() == 2);
    CHECK(reg.get("test") != nullptr);
    CHECK(reg.get("d") != nullptr);
}
