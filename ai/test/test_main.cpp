#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <silicon/test/test.hpp>

// ── 组合根（composition root）────────────────────────────────────
// 顶层 exe 拥有 llm 专属 error_category 的唯一实例，并经 DI 注入；
// 下层（silicon.ai.llm 模块内部）统一经 llm_category() 引用同一对象，
// 满足 std::error_category「全局唯一地址」契约，跨模块 / 跨 DLL 一致。
// 真实实例在最上层 exe 构造，其他模块/库/DLL 皆经注入句柄收到同一对象。
import silicon.ai.llm.error;

namespace {
    const silicon::ai::llm::llm_category_impl g_llm_category;
    struct inject_llm_category_on_init {
        inject_llm_category_on_init() {
            silicon::ai::llm::inject_llm_category(g_llm_category);
        }
    } _inject_llm_category_on_init;
}
