module;

#include <expected>
#include <string>
#include <system_error>

export module silicon.config.error;

import silicon.error;

export namespace silicon::config {

/// 统一错误返回类型：config 模块可失败 API 返回 config::result<T>。
/// 转发至 silicon.error 的集中别名。
template<typename T>
using result = silicon::error::result<T>;

/// config 模块专属错误码枚举（加载 / 解析失败）。
enum class config_error {
    kLoadFailed = 1,
    kParseFailed,
    kInvalidValue,
    kUnknown,
};

/// 返回 config_error 专属 error_category（name() = "silicon.config"）。
[[nodiscard]] inline const std::error_category &config_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.config"; }
        std::string message(int ev) const override {
            switch(static_cast<config_error>(ev)) {
                case config_error::kLoadFailed: return "config load failed";
                case config_error::kParseFailed: return "config parse failed";
                case config_error::kInvalidValue: return "invalid config value";
                case config_error::kUnknown: return "unknown config error";
            }
            return "unknown config error";
        }
    } cat;
    return cat;
}

/// 将 config_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(config_error e) noexcept {
    return {static_cast<int>(e), config_category()};
}

} // namespace silicon::config
