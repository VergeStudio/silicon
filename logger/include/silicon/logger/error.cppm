module;

#include <string>
#include <system_error>

export module silicon.logger.error;

export namespace silicon::logger {

/// logger 模块专属错误码枚举。
enum class logger_error {
    kInitFailed = 1,
    kInvalidLevel,
};

/// 返回 logger_error 专属 error_category（name() = "silicon.logger"）。
[[nodiscard]] inline const std::error_category &logger_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.logger"; }
        std::string message(int ev) const override {
            switch(static_cast<logger_error>(ev)) {
                case logger_error::kInitFailed: return "logger init failed";
                case logger_error::kInvalidLevel: return "invalid log level";
            }
            return "unknown logger error";
        }
    } cat;
    return cat;
}

/// 将 logger_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(logger_error e) noexcept {
    return {static_cast<int>(e), logger_category()};
}

} // namespace silicon::logger
