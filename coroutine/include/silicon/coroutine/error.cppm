module;

#include <exception>
#include <memory>
#include <string>
#include <system_error>

export module silicon.coroutine.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::coroutine {

inline std::unique_ptr<const std::error_category, silicon::error::category_deleter>
    coroutine_error_category_instance;
inline std::unique_ptr<const std::error_category, silicon::error::category_deleter>
    channel_error_category_instance;

} // namespace silicon::coroutine

export namespace silicon::coroutine {

/// coroutine 模块专属错误码枚举（同步原语与协程池）。
enum class coroutine_error {
    kNullExecutor = 1,
    kInvalidPoolSize,
    kAlreadyUnlocked,
    kUnknown,
};

/// coroutine::channel / queue / ring_buffer 专属错误码枚举。
enum class channel_error {
    kClosed = 1,
    kTimeout,
    kCancelled,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
class coroutine_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.coroutine"; }
    std::string message(int ev) const override {
        switch(static_cast<coroutine_error>(ev)) {
            case coroutine_error::kNullExecutor: return "executor must not be null";
            case coroutine_error::kInvalidPoolSize: return "pool size must be greater than zero";
            case coroutine_error::kAlreadyUnlocked: return "mutex is already unlocked";
            case coroutine_error::kUnknown: return "unknown coroutine error";
        }
        return "unknown coroutine error";
    }
};

class channel_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.channel"; }
    std::string message(int ev) const override {
        switch(static_cast<channel_error>(ev)) {
            case channel_error::kClosed: return "channel closed";
            case channel_error::kTimeout: return "operation timed out";
            case channel_error::kCancelled: return "operation cancelled";
        }
        return "unknown channel error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_coroutine_error_category(const std::error_category &cat) noexcept {
    coroutine_error_category_instance.reset(&cat);
}

inline void inject_channel_error_category(const std::error_category &cat) noexcept {
    channel_error_category_instance.reset(&cat);
}

/// 返回 coroutine_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &coroutine_category() noexcept {
    if (!coroutine_error_category_instance) {
        std::terminate();
    }
    return *coroutine_error_category_instance;
}

/// 返回 channel_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &channel_category() noexcept {
    if (!channel_error_category_instance) {
        std::terminate();
    }
    return *channel_error_category_instance;
}

/// 将 coroutine_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(coroutine_error e) noexcept {
    return {static_cast<int>(e), coroutine_category()};
}

/// 将 channel_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(channel_error e) noexcept {
    return {static_cast<int>(e), channel_category()};
}

} // namespace silicon::coroutine
