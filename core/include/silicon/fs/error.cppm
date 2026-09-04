module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>

export module silicon.fs.error;

import silicon.error;

export namespace silicon::fs {

CORE_API std::atomic<const std::error_category *> fs_error_category_instance{nullptr};

}

export namespace silicon::fs {

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

inline void inject_fs_error_category(const std::error_category &cat) noexcept {
    fs_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &fs_category() noexcept {
    const std::error_category *cat = fs_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(fs_error e) noexcept {
    return {static_cast<int>(e), fs_category()};
}

}
