#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.h>

// ── 组合根（composition root）────────────────────────────────────
// 顶层 exe 拥有 fs 专属 error_category 的唯一实例，并经 DI 注入；
// 下层（silicon.fs 模块内部）统一经 fs_category() 引用同一对象，
// 满足 std::error_category「全局唯一地址」契约，跨模块 / 跨 DLL 一致。
import silicon.fs.error;

namespace {
    const silicon::fs::fs_category_impl g_fs_category;
    struct inject_fs_on_init {
        inject_fs_on_init() {
            silicon::fs::inject_fs_error_category(g_fs_category);
        }
    } _inject_fs_on_init;
}
