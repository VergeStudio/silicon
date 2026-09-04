module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.scheduler.error;

import silicon.error;

namespace silicon::scheduler {

CORE_API std::atomic<const std::error_category *> scheduler_error_category_instance{nullptr};

}

export namespace silicon::scheduler {

enum class scheduler_error {
    kShuttingDown = 1,
    kResultNotSet,
    kInvalidNotifierState,
    kPipeCreateFailed,
    kEventRegisterFailed,
    kNullExecutor,
    kUnknown,

    kNoCompletionBackend,
    kNotRegularFile,
    kCompletionSubmitFailed,
};

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
            case scheduler_error::kNoCompletionBackend:
                return "no completion I/O backend is available";
            case scheduler_error::kNotRegularFile: return "file descriptor is not a regular file";
            case scheduler_error::kCompletionSubmitFailed:
                return "failed to submit completion I/O operation";
            case scheduler_error::kUnknown: return "unknown scheduler error";
        }
        return "unknown scheduler error";
    }
};

inline void inject_scheduler_error_category(const std::error_category &cat) noexcept {
    scheduler_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &scheduler_category() noexcept {
    const std::error_category *cat = scheduler_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(scheduler_error e) noexcept {
    return {static_cast<int>(e), scheduler_category()};
}

}

namespace {
    const silicon::scheduler::scheduler_category_impl s_default_scheduler_category{};
    const bool s_scheduler_category_registered = [] {
        silicon::scheduler::inject_scheduler_error_category(s_default_scheduler_category);
        return true;
    }();
}
