module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>
#include <silicon/common.h>

export module silicon.logger.error;

import silicon.error;

export namespace silicon::logger {

// 错误类别实例（组合根注入）：跨 DLL 消费方经导出的内联函数 logger_category() /
// make_error_code() 引用，须以 LOGGER_API 显式导出，否则 LNK2001。
LOGGER_API std::atomic<const std::error_category *> logger_error_category_instance{nullptr};

/// logger 模块专属错误码枚举。
enum class logger_error {
    kInitFailed = 1,
    kInvalidLevel,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
// vtable 须以 LOGGER_API 显式导出，否则跨 DLL 消费方出现 LNK2001。
class LOGGER_API logger_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.logger"; }
    std::string message(int ev) const override {
        switch(static_cast<logger_error>(ev)) {
            case logger_error::kInitFailed: return "logger init failed";
            case logger_error::kInvalidLevel: return "invalid log level";
        }
        return "unknown logger error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_logger_error_category(const std::error_category &cat) noexcept {
    logger_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 logger_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &logger_category() noexcept {
    const std::error_category *cat = logger_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 logger_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(logger_error e) noexcept {
    return {static_cast<int>(e), logger_category()};
}

} // namespace silicon::logger
