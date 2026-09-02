module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>
#include <silicon/common.h>

export module silicon.di.error;

import silicon.error;

export namespace silicon::di {

// 错误类别实例（组合根注入）：跨 DLL 消费方经导出的内联函数 di_category() /
// make_error_code() 引用，须以 CORE_API 显式导出，否则 LNK2001。
CORE_API std::atomic<const std::error_category *> di_error_category_instance{nullptr};

/// di 模块专属错误码枚举（依赖注入容器 resolve 链路）。
enum class di_error {
    kDuplicateBinding = 1,
    kUnresolvedDependency,
    kCircularDependency,
    kInvalidType,
    kAlreadyInitialized,
    kTypeNotFound,
    kTypeAmbiguous,
    kTypeNotConvertible,
    kTypeRecursion,
    kTypeAlreadyRegistered,
    kTypeIndexAlreadyRegistered,
    kCollectionTypeNotFound,
    kIndexOutOfRange,
    kUnknown,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
// vtable 须以 CORE_API 显式导出，否则跨 DLL 消费方出现 LNK2001。
class CORE_API di_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.di"; }
    std::string message(int ev) const override {
        switch(static_cast<di_error>(ev)) {
            case di_error::kDuplicateBinding: return "duplicate binding";
            case di_error::kUnresolvedDependency: return "unresolved dependency";
            case di_error::kCircularDependency: return "circular dependency detected";
            case di_error::kInvalidType: return "invalid type";
            case di_error::kAlreadyInitialized: return "already initialized";
            case di_error::kTypeNotFound: return "requested type not found in container";
            case di_error::kTypeAmbiguous: return "requested type resolves ambiguously";
            case di_error::kTypeNotConvertible: return "registered type is not convertible to requested type";
            case di_error::kTypeRecursion: return "recursive type resolution detected";
            case di_error::kTypeAlreadyRegistered: return "type already registered";
            case di_error::kTypeIndexAlreadyRegistered: return "type index already registered";
            case di_error::kCollectionTypeNotFound: return "collection element type not found";
            case di_error::kIndexOutOfRange: return "type index out of range";
            case di_error::kUnknown: return "unknown di error";
        }
        return "unknown di error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_di_error_category(const std::error_category &cat) noexcept {
    di_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 di_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &di_category() noexcept {
    const std::error_category *cat = di_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 di_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(di_error e) noexcept {
    return {static_cast<int>(e), di_category()};
}

} // namespace silicon::di

// category 自注册：与 silicon.network / silicon.config / silicon.logger / silicon.event /
// silicon.library 一致，避免未注入消费方走错误路径时 di_category() 直接 std::terminate
// （resolve/construct 失败均经 make_error_code 触发）。匿名命名空间须置于模块作用域
// （export 块之外），否则 clang 报 "anonymous namespaces cannot be exported"。组合根仍可
// 调 inject_di_error_category 注入自定义实例。
namespace {
    const silicon::di::di_category_impl s_default_di_category{};
    const bool s_di_category_registered = [] {
        silicon::di::inject_di_error_category(s_default_di_category);
        return true;
    }();
} // namespace
