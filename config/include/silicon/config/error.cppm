module;

#include <atomic>
#include <expected>
#include <exception>
#include <string>
#include <system_error>

export module silicon.config.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::config {

std::atomic<const std::error_category *> config_error_category_instance{nullptr};

} // namespace silicon::config

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

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
class config_category_impl final : public std::error_category {
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
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_config_error_category(const std::error_category &cat) noexcept {
    config_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 config_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &config_category() noexcept {
    const std::error_category *cat = config_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 config_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(config_error e) noexcept {
    return {static_cast<int>(e), config_category()};
}

} // namespace silicon::config
