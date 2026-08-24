module;

#include <string>
#include <system_error>

export module silicon.ai.llm.error;

export namespace silicon::ai::llm {

/// LLM 语义错误枚举（专属 category：silicon.ai）。
enum class llm_error {
    kProviderUnavailable = 1,
    kInvalidResponse,
    kToolNotFound,
    kTimeout,
    kUnknown,
};

// Named class instead of an anonymous-class local static: MSVC module builds
// mishandle the vtable of an anonymous derived class inside an inline function
// local static (name()/message() virtual dispatch crashes with SIGSEGV).
// 该实现类由组合根（composition root）实例化并经 inject_llm_category 注入，
// 模块自身不再持有单例；下层统一经 llm_category() 取到同一实例，跨模块/跨 DLL 安全。
export class llm_category_impl final: public std::error_category {
    const char *name() const noexcept override { return "silicon.ai"; }
    std::string message(int ev) const override {
        switch(static_cast<llm_error>(ev)) {
            case llm_error::kProviderUnavailable:
                return "llm provider unavailable";
            case llm_error::kInvalidResponse:
                return "invalid llm response";
            case llm_error::kToolNotFound:
                return "tool not found";
            case llm_error::kTimeout:
                return "llm request timed out";
            case llm_error::kUnknown:
                return "unknown llm error";
        }
        return "unknown llm error";
    }
};

// ---- 依赖注入把手 ----
// 组合根在程序启动期构造唯一 llm_category_impl 实例并注入；下层所有代码
// 经 llm_category() 取到同一地址，满足 std::error_category「全局唯一地址」契约，
// 跨模块 / 跨 DLL 安全（进程统一虚拟地址空间，同一指针处处相同）。
// 以 export inline 全局承载：导出 inline 变量具外部链接、程序内单一定义，
// 可被导出 inline 的 inject/llm_category 在导入方安全引用（避 module-linkage 链接失败）。
export inline const std::error_category *llm_category_instance = nullptr;

/// 组合根注入全局唯一 category 实例（必须在任何 make_error_code 调用之前完成）。
export inline void inject_llm_category(const std::error_category &cat) noexcept {
    llm_category_instance = &cat;
}

/// 返回 llm_error 专属 error_category（优先注入实例，否则退化为模块内 Meyers 单例）。
/// 退化路径仅当「从未注入」时生效；一旦注入发生在首次使用之前，则全程序单一实例。
[[nodiscard]] inline const std::error_category &llm_category() noexcept {
    if (llm_category_instance != nullptr) {
        return *llm_category_instance;
    }
    static const llm_category_impl fallback;
    return fallback;
}

/// llm_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(llm_error e) noexcept {
    return {static_cast<int>(e), llm_category()};
}

} // namespace silicon::ai::llm
