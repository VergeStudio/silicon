module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

// CORE_API 宏（dllexport/dlimport 闸门）：fs_category_impl 的 vtable 须跨 DLL 导出，
// 否则消费方（测试/应用）内联 make_error_code / fs_category 时无法解析其虚函数槽。
#include <silicon/common.h>

export module silicon.fs.error;

import silicon.error;

// ---- 跨 DLL 共享的 DI 句柄：须显式 dllexport（CORE_API），否则被内联进消费方的
// inject_fs_error_category / fs_category / make_error_code 无法解析其实例地址（LNK2001）。
// 模块链接的 export 变量在直接编译进 DLL 时不会自动导出到导入库，须 *API 强标。----
export namespace silicon::fs {

CORE_API std::atomic<const std::error_category *> fs_error_category_instance{nullptr};

} // namespace silicon::fs

export namespace silicon::fs {

/// 文件系统语义错误枚举（专属 category：silicon.fs）。
enum class fs_error {
    kOpenFailed = 1,
    kReadFailed,
    kWriteFailed,
    kNotExist,
    kPermissionDenied,
    kNotDirectory,
    kAlreadyExists,
    kUnknown,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
// CORE_API 强制导出其 vtable（name/message 虚函数槽），否则跨 DLL 消费方解析失败。
class CORE_API fs_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.fs"; }
    std::string message(int ev) const override {
        switch(static_cast<fs_error>(ev)) {
            case fs_error::kOpenFailed: return "cannot open file";
            case fs_error::kReadFailed: return "cannot read file";
            case fs_error::kWriteFailed: return "cannot write file";
            case fs_error::kNotExist: return "file or directory does not exist";
            case fs_error::kPermissionDenied: return "permission denied";
            case fs_error::kNotDirectory: return "not a directory";
            case fs_error::kAlreadyExists: return "file or directory already exists";
            case fs_error::kUnknown: return "unknown file system error";
        }
        return "unknown fs error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_fs_error_category(const std::error_category &cat) noexcept {
    fs_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 fs_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &fs_category() noexcept {
    const std::error_category *cat = fs_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// fs_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(fs_error e) noexcept {
    return {static_cast<int>(e), fs_category()};
}

} // namespace silicon::fs
