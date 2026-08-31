module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include "silicon/coroutine/common.h"

export module silicon.coroutine.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::coroutine {

// 并入 core.dll 后：变量不随 DLL 自动导出（MSVC），且 test / 外部消费方经
// inline 注入函数展开会跨 DLL 引用本原子变量 → 标 COROUTINE_API（dllexport）。
COROUTINE_API std::atomic<const std::error_category *> coroutine_error_category_instance{nullptr};
COROUTINE_API std::atomic<const std::error_category *> channel_error_category_instance{nullptr};

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
// 并入 core.dll 后 inline-virtual 类的 vtable 不随 DLL 自动导出 → 类级标注
// （宏须在 class 关键字后，规避 C4091）。
class COROUTINE_API coroutine_category_impl final : public std::error_category {
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

class COROUTINE_API channel_category_impl final : public std::error_category {
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
    coroutine_error_category_instance.store(&cat, std::memory_order_release);
}

inline void inject_channel_error_category(const std::error_category &cat) noexcept {
    channel_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 coroutine_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &coroutine_category() noexcept {
    const std::error_category *cat = coroutine_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 返回 channel_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &channel_category() noexcept {
    const std::error_category *cat = channel_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
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
