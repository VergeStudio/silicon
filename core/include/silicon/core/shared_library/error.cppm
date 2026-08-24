module;

#include <exception>
#include <memory>
#include <string>
#include <system_error>

export module silicon.library.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::library {

inline std::unique_ptr<const std::error_category, silicon::error::category_deleter> library_error_category_instance;

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
class library_category_impl final : public std::error_category {
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
    library_error_category_instance.reset(&cat);
}

/// 返回 library_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &library_category() noexcept {
    if (!library_error_category_instance) {
        std::terminate();
    }
    return *library_error_category_instance;
}

/// library_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(library_error e) noexcept {
    return {static_cast<int>(e), library_category()};
}

} // namespace silicon::library
