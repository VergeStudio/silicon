module;

#include <string>
#include <system_error>

export module silicon.http.error;

export namespace silicon::http {

/// http 模块专属错误码枚举。
enum class http_error {
    kRequestFailed = 1,
    kInvalidResponse,
    kTimeout,
    kUnknown,
};

/// 返回 http_error 专属 error_category（name() = "silicon.http"）。
[[nodiscard]] inline const std::error_category &http_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.http"; }
        std::string message(int ev) const override {
            switch(static_cast<http_error>(ev)) {
                case http_error::kRequestFailed: return "http request failed";
                case http_error::kInvalidResponse: return "invalid http response";
                case http_error::kTimeout: return "http request timed out";
                case http_error::kUnknown: return "unknown http error";
            }
            return "unknown http error";
        }
    } cat;
    return cat;
}

/// 将 http_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(http_error e) noexcept {
    return {static_cast<int>(e), http_category()};
}

} // namespace silicon::http
