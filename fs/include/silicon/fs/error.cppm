module;

#include <exception>
#include <memory>
#include <string>
#include <system_error>

export module silicon.fs.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::fs {

inline std::unique_ptr<const std::error_category, silicon::error::category_deleter> fs_error_category_instance;

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
class fs_category_impl final : public std::error_category {
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
    fs_error_category_instance.reset(&cat);
}

/// 返回 fs_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &fs_category() noexcept {
    if (!fs_error_category_instance) {
        std::terminate();
    }
    return *fs_error_category_instance;
}

/// fs_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(fs_error e) noexcept {
    return {static_cast<int>(e), fs_category()};
}

} // namespace silicon::fs
