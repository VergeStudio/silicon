#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.h>

// ── 组合根（composition root）────────────────────────────────────
// 顶层 exe 拥有 coroutine / channel 专属 error_category 的唯一实例，并经 DI 注入；
// 下层（silicon.coroutine 模块内部）统一经 coroutine_category() / channel_category()
// 引用同一对象，满足 std::error_category「全局唯一地址」契约，跨模块 / 跨 DLL 一致。
import silicon.coroutine.error;

namespace {
    const silicon::coroutine::coroutine_category_impl g_coroutine_category;
    const silicon::coroutine::channel_category_impl g_channel_category;
    struct inject_coroutine_on_init {
        inject_coroutine_on_init() {
            silicon::coroutine::inject_coroutine_error_category(g_coroutine_category);
            silicon::coroutine::inject_channel_error_category(g_channel_category);
        }
    } _inject_coroutine_on_init;
}
