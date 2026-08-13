module;

#include <string>
#include <system_error>

export module silicon.fs.error;

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

/// 返回 fs_error 专属 error_category（name() == "silicon.fs"）。
[[nodiscard]] inline const std::error_category &fs_category() noexcept {
    static const class : public std::error_category {
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
    } cat;
    return cat;
}

/// fs_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(fs_error e) noexcept {
    return {static_cast<int>(e), fs_category()};
}

} // namespace silicon::fs
