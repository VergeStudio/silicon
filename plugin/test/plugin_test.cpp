#include <memory>
#include <silicon/test/test.hpp>
#include <string>
import silicon.plugin;
using namespace silicon::plugin;

namespace {
struct TestPlugin: IPlugin {
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
} // namespace

TEST_CASE("PluginRegistry: 注册与查询") {
    PluginRegistry reg;
    auto p = std::make_shared<TestPlugin>();
    CHECK(reg.register_plugin(p));
    CHECK(reg.get_plugin("test") == p.get());
    CHECK(reg.list_plugins().size() == 1);
}

TEST_CASE("PluginRegistry: 重复注册失败") {
    PluginRegistry reg;
    auto p1 = std::make_shared<TestPlugin>();
    auto p2 = std::make_shared<TestPlugin>();
    CHECK(reg.register_plugin(p1));
    CHECK_FALSE(reg.register_plugin(p2)); // same name "test"
}

TEST_CASE("PluginRegistry: 移除触发 on_unload") {
    PluginRegistry reg;
    auto p = std::make_shared<TestPlugin>();
    reg.register_plugin(p);
    CHECK(reg.remove_plugin("test"));
    CHECK(reg.get_plugin("test") == nullptr);
}
