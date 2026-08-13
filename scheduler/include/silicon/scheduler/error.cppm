module;

#include <string>
#include <system_error>

export module silicon.scheduler.error;

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

/// 返回 scheduler_error 专属 error_category（name() = "silicon.scheduler"）。
[[nodiscard]] inline const std::error_category &scheduler_category() noexcept {
    static const class : public std::error_category {
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
    } cat;
    return cat;
}

/// 将 scheduler_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(scheduler_error e) noexcept {
    return {static_cast<int>(e), scheduler_category()};
}

} // namespace silicon::scheduler
