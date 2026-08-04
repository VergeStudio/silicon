#include <memory>
#include <silicon/test/test.hpp>
#include <string>
import silicon.plugin;
using namespace silicon::plugin;

namespace {
struct TestPlugin: Plugin {
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
} // namespace

TEST_CASE("DefaultPluginRegistry: 注册与查询") {
    DefaultPluginRegistry reg;
    auto p = std::make_shared<TestPlugin>();
    CHECK(reg.RegisterPlugin(p));
    CHECK(reg.GetPlugin("test") == p.get());
    CHECK(reg.ListPlugins().size() == 1);
}

TEST_CASE("DefaultPluginRegistry: 重复注册失败") {
    DefaultPluginRegistry reg;
    auto p1 = std::make_shared<TestPlugin>();
    auto p2 = std::make_shared<TestPlugin>();
    CHECK(reg.RegisterPlugin(p1));
    CHECK_FALSE(reg.RegisterPlugin(p2)); // same name "test"
}

TEST_CASE("DefaultPluginRegistry: 移除触发 OnUnload") {
    DefaultPluginRegistry reg;
    auto p = std::make_shared<TestPlugin>();
    reg.RegisterPlugin(p);
    CHECK(reg.RemovePlugin("test"));
    CHECK(reg.GetPlugin("test") == nullptr);
}
