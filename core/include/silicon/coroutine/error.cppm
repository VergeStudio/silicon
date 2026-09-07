module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include "silicon/common.h"

export module silicon.coroutine.error;

import silicon.error;

namespace silicon::coroutine {

SILICON_CORE_API std::atomic<const std::error_category *> coroutine_error_category_instance{nullptr};
SILICON_CORE_API std::atomic<const std::error_category *> channel_error_category_instance{nullptr};

}

export namespace silicon::coroutine {

enum class coroutine_error {
    kNullExecutor = 1,
    kInvalidPoolSize,
    kAlreadyUnlocked,
    kUnknown,
};

enum class channel_error {
    kClosed = 1,
    kTimeout,
    kCancelled,
};

class SILICON_CORE_API coroutine_category_impl final : public std::error_category {
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

class SILICON_CORE_API channel_category_impl final : public std::error_category {
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

inline void inject_coroutine_error_category(const std::error_category &cat) noexcept {
    coroutine_error_category_instance.store(&cat, std::memory_order_release);
}

inline void inject_channel_error_category(const std::error_category &cat) noexcept {
    channel_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &coroutine_category() noexcept {
    const std::error_category *cat = coroutine_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline const std::error_category &channel_category() noexcept {
    const std::error_category *cat = channel_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(coroutine_error e) noexcept {
    return {static_cast<int>(e), coroutine_category()};
}

[[nodiscard]] inline std::error_code make_error_code(channel_error e) noexcept {
    return {static_cast<int>(e), channel_category()};
}

}
