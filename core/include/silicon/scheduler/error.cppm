module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.scheduler.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::scheduler {

CORE_API std::atomic<const std::error_category *> scheduler_error_category_instance{nullptr};

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
class CORE_API scheduler_category_impl final : public std::error_category {
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
    scheduler_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 scheduler_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &scheduler_category() noexcept {
    const std::error_category *cat = scheduler_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 scheduler_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(scheduler_error e) noexcept {
    return {static_cast<int>(e), scheduler_category()};
}

} // namespace silicon::scheduler

// category 自注册：与 silicon.network / silicon.config / silicon.logger / silicon.event /
// silicon.library / silicon.di 一致，避免未注入消费方走错误路径时 scheduler_category()
// 直接 std::terminate（io_scheduler / thread_pool 的构造失败与关闭路径均经
// make_error_code 触发）。匿名命名空间须置于模块作用域（export 块之外），否则 clang 报
// "anonymous namespaces cannot be exported"。组合根仍可调
// inject_scheduler_error_category 注入自定义实例（原子存储，后写覆盖）。
namespace {
    const silicon::scheduler::scheduler_category_impl s_default_scheduler_category{};
    const bool s_scheduler_category_registered = [] {
        silicon::scheduler::inject_scheduler_error_category(s_default_scheduler_category);
        return true;
    }();
} // namespace
