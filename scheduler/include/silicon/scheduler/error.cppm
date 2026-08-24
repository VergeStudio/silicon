module;

#include <exception>
#include <memory>
#include <string>
#include <system_error>

export module silicon.scheduler.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::scheduler {

inline std::unique_ptr<const std::error_category, silicon::error::category_deleter> scheduler_error_category_instance;

} // namespace silicon::scheduler

export namespace silicon::scheduler {

/// scheduler 模块专属错误码枚举（执行器 / IO 通知器 / 线程池）。
enum class scheduler_error {
    kShuttingDown = 1,
    kResultNotSet,
    kInvalidNotifierState,
    kPipeCreateFailed,
    kEventRegisterFailed,
    kNullExecutor,
    kUnknown,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
class scheduler_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.scheduler"; }
    std::string message(int ev) const override {
        switch(static_cast<scheduler_error>(ev)) {
            case scheduler_error::kShuttingDown: return "scheduler is shutting down";
            case scheduler_error::kResultNotSet:
                return "coroutine result was never set, did you execute the coroutine?";
            case scheduler_error::kInvalidNotifierState: return "invalid io notifier state";
            case scheduler_error::kPipeCreateFailed: return "failed to create pipe";
            case scheduler_error::kEventRegisterFailed: return "failed to register event";
            case scheduler_error::kNullExecutor: return "executor must not be null";
            case scheduler_error::kUnknown: return "unknown scheduler error";
        }
        return "unknown scheduler error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_scheduler_error_category(const std::error_category &cat) noexcept {
    scheduler_error_category_instance.reset(&cat);
}

/// 返回 scheduler_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &scheduler_category() noexcept {
    if (!scheduler_error_category_instance) {
        std::terminate();
    }
    return *scheduler_error_category_instance;
}

/// 将 scheduler_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(scheduler_error e) noexcept {
    return {static_cast<int>(e), scheduler_category()};
}

} // namespace silicon::scheduler
