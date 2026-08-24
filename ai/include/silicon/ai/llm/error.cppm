module;

#include <exception>
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
class llm_category_impl final: public std::error_category {
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
// llm_category_instance 是模块内部实现细节：模块链接、模块内单一定义，不导出。
// 消费方只经 inject_llm_category() 写入、经 llm_category() 读取，不可直接触碰；
// 句柄唯一性由「承载本 TU 的模块以 shared 方式进程内只加载一次」保证，与变量
// 是否 export（外部链接可见性）无关。
inline const std::error_category *llm_category_instance = nullptr;

/// 组合根注入全局唯一 category 实例（必须在任何 make_error_code 调用之前完成）。
inline void inject_llm_category(const std::error_category &cat) noexcept {
    llm_category_instance = &cat;
}

/// 返回 llm_error 专属 error_category。
/// DI 是唯一来源：组合根必须在首次使用前经 inject_llm_category 注入唯一实例。
/// 不提供模块内 fallback 单例——否则会破坏 std::error_category「全局唯一地址」契约，
/// 并在跨 DLL / 多二进制场景下重现重复单例问题。未注入即使用属组合根接线错误，直接终止。
[[nodiscard]] inline const std::error_category &llm_category() noexcept {
    if (llm_category_instance == nullptr) {
        std::terminate();
    }
    return *llm_category_instance;
}

/// llm_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(llm_error e) noexcept {
    return {static_cast<int>(e), llm_category()};
}

} // namespace silicon::ai::llm
