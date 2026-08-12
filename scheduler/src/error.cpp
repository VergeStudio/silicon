// 实现单元：silicon.scheduler —— scheduler_error 的 category 与 make_error_code 定义。
// name() = "silicon.scheduler"。

module;

#include <string>
#include <system_error>

module silicon.scheduler;

namespace silicon::scheduler {
namespace {

class scheduler_error_category final : public std::error_category {
  public:
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

} // namespace

const std::error_category &scheduler_category() noexcept {
    static const scheduler_error_category cat{};
    return cat;
}

std::error_code make_error_code(scheduler_error e) noexcept {
    return {static_cast<int>(e), scheduler_category()};
}

} // namespace silicon::scheduler
