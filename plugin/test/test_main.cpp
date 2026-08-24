#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.hpp>

// ── 组合根（composition root）────────────────────────────────────
// 顶层 exe 拥有 plugin 专属 error_category 的唯一实例，并经 DI 注入；
// 下层（silicon.plugin 模块内部）统一经 plugin_category() 引用同一对象，
// 满足 std::error_category「全局唯一地址」契约，跨模块 / 跨 DLL 一致。
import silicon.plugin.error;

namespace {
    const silicon::plugin::plugin_category_impl g_plugin_category;
    struct inject_plugin_on_init {
        inject_plugin_on_init() {
            silicon::plugin::inject_plugin_error_category(g_plugin_category);
        }
    } _inject_plugin_on_init;
}
