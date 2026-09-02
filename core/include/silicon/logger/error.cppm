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
// make_error_code() 引用，须以 CORE_API 显式导出，否则 LNK2001。
CORE_API std::atomic<const std::error_category *> logger_error_category_instance{nullptr};

/// logger 模块专属错误码枚举。
enum class logger_error {
    kInitFailed = 1,
    kInvalidLevel,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
// vtable 须以 CORE_API 显式导出，否则跨 DLL 消费方出现 LNK2001。
class CORE_API logger_category_impl final : public std::error_category {
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

// category 自注册：与 silicon.network / silicon.config 一致，避免未注入消费方走错误路径时
// logger_category() 直接 std::terminate（init 失败经 make_error_code 构造 kInitFailed 即触发）。
// 匿名命名空间须置于模块作用域（export 块之外），否则 clang 报
// "anonymous namespaces cannot be exported"。组合根仍可调 inject_logger_error_category 注入自定义实例。
namespace {
    const silicon::logger::logger_category_impl s_default_logger_category{};
    const bool s_logger_category_registered = [] {
        silicon::logger::inject_logger_error_category(s_default_logger_category);
        return true;
    }();
} // namespace
