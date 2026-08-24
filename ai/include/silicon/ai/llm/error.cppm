module;

#include <exception>
#include <memory>
#include <string>
#include <system_error>

export module silicon.ai.llm.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出） ----
// 置于 export namespace 之外，使其具模块链接而非外部链接（消费方不可直触）。
namespace silicon::ai::llm {

// 用 silicon.error 提供的 no-op 删除器承载「模块独占 category 句柄」语义而不实际 delete
// （std::error_category 析构为保护、进程期常驻；真实生命周期由组合根持有）。
inline std::unique_ptr<const std::error_category, silicon::error::category_deleter> llm_error_category_instance;

} // namespace silicon::ai::llm

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
    const char *name() const noexcept override { return "silicon.ai.llm"; }
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

/// 组合根注入全局唯一 category 实例（必须在任何 make_error_code 调用之前完成）。
/// 注入后模块独占该句柄；std::error_category 设计上进程期常驻，故不释放。
inline void inject_llm_error_category(const std::error_category &cat) noexcept {
    llm_error_category_instance.reset(&cat);
}

/// 返回 llm_error 专属 error_category。
/// DI 是唯一来源：组合根必须先注入；未注入即使用属组合根接线错误，直接终止。
/// 不提供模块内 fallback 单例——否则会破坏 std::error_category「全局唯一地址」契约，
/// 并在跨 DLL / 多二进制场景下重现重复单例问题。
[[nodiscard]] inline const std::error_category &llm_error_category() noexcept {
    if (!llm_error_category_instance) {
        std::terminate();
    }
    return *llm_error_category_instance;
}

/// llm_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(llm_error e) noexcept {
    return {static_cast<int>(e), llm_error_category()};
}

} // namespace silicon::ai::llm
