module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

// CORE_API 宏（dllexport/dlimport 闸门）：library_category_impl 的 vtable 须跨 DLL 导出，
// 否则消费方（测试/应用）内联 make_error_code / library_category 时无法解析其虚函数槽。
#include <silicon/core/common.h>

export module silicon.library.error;

import silicon.error;

// ---- 跨 DLL 共享的 DI 句柄：须显式 dllexport（CORE_API），否则被内联进消费方的
// inject_library_error_category / library_category / make_error_code 无法解析其实例地址（LNK2001）。
// 模块链接的 export 变量在直接编译进 DLL 时不会自动导出到导入库，须 *API 强标。----
export namespace silicon::library {

CORE_API std::atomic<const std::error_category *> library_error_category_instance{nullptr};

} // namespace silicon::library

export namespace silicon::library {

/// 动态库语义错误枚举（专属 category：silicon.library）。
enum class library_error {
    kAlreadyLoaded = 1,
    kLoadFailed,
    kUnloadFailed,
    kSymbolNotFound,
    kInvalidHandle,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
// CORE_API 强制导出其 vtable（name/message 虚函数槽），否则跨 DLL 消费方解析失败。
class CORE_API library_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.library"; }
    std::string message(int ev) const override {
        switch(static_cast<library_error>(ev)) {
            case library_error::kAlreadyLoaded: return "library already loaded";
            case library_error::kLoadFailed: return "failed to load shared library";
            case library_error::kUnloadFailed: return "failed to unload shared library";
            case library_error::kSymbolNotFound: return "symbol not found in shared library";
            case library_error::kInvalidHandle: return "invalid shared library handle";
        }
        return "unknown library error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_library_error_category(const std::error_category &cat) noexcept {
    library_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 library_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &library_category() noexcept {
    const std::error_category *cat = library_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// library_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(library_error e) noexcept {
    return {static_cast<int>(e), library_category()};
}

} // namespace silicon::library
