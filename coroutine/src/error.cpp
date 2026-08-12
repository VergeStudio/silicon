// 实现单元：silicon.coroutine —— coroutine_error / channel_error 的 category
// 与 make_error_code 定义。name() 分别为 "silicon.coroutine" 与 "silicon.channel"。

module;

#include <string>
#include <system_error>

module silicon.coroutine;

namespace silicon::coroutine {
namespace {

class coroutine_error_category final : public std::error_category {
  public:
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

class channel_error_category final : public std::error_category {
  public:
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

} // namespace

const std::error_category &coroutine_category() noexcept {
    static const coroutine_error_category cat{};
    return cat;
}

const std::error_category &channel_category() noexcept {
    static const channel_error_category cat{};
    return cat;
}

std::error_code make_error_code(coroutine_error e) noexcept {
    return {static_cast<int>(e), coroutine_category()};
}

std::error_code make_error_code(channel_error e) noexcept {
    return {static_cast<int>(e), channel_category()};
}

} // namespace silicon::coroutine
