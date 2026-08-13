module;

#include <string>
#include <system_error>

export module silicon.library.error;

export namespace silicon::library {

/// 动态库语义错误枚举（专属 category：silicon.library）。
enum class library_error {
    kAlreadyLoaded = 1,
    kLoadFailed,
    kUnloadFailed,
    kSymbolNotFound,
    kInvalidHandle,
};

/// 返回 library_error 专属 error_category（name() == "silicon.library"）。
[[nodiscard]] inline const std::error_category &library_category() noexcept {
    static const class : public std::error_category {
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
    } cat;
    return cat;
}

/// library_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(library_error e) noexcept {
    return {static_cast<int>(e), library_category()};
}

} // namespace silicon::library
