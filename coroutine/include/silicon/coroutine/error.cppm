module;

#include <string>
#include <system_error>

export module silicon.coroutine.error;

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

/// 返回 coroutine_error 专属 error_category（name() = "silicon.coroutine"）。
[[nodiscard]] inline const std::error_category &coroutine_category() noexcept {
    static const class : public std::error_category {
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
    } cat;
    return cat;
}

/// 返回 channel_error 专属 error_category（name() = "silicon.channel"）。
[[nodiscard]] inline const std::error_category &channel_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.channel"; }
        std::string message(int ev) const override {
            switch(static_cast<channel_error>(ev)) {
                case channel_error::kClosed: return "channel closed";
                case channel_error::kTimeout: return "operation timed out";
                case channel_error::kCancelled: return "operation cancelled";
            }
            return "unknown channel error";
        }
    } cat;
    return cat;
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
