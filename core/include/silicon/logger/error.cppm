module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>
#include <silicon/common.h>

export module silicon.logger.error;

import silicon.error;

export namespace silicon::logger {

SILICON_CORE_API std::atomic<const std::error_category *> logger_error_category_instance{nullptr};

enum class logger_error {
    kInitFailed = 1,
    kInvalidLevel,
};

class SILICON_CORE_API logger_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.logger"; }
    std::string message(int ev) const override {
        switch(static_cast<logger_error>(ev)) {
            case logger_error::kInitFailed: return "logger init failed";
            case logger_error::kInvalidLevel: return "invalid log level";
        }
        return "unknown logger error";
    }
};

inline void inject_logger_error_category(const std::error_category &cat) noexcept {
    logger_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &logger_category() noexcept {
    const std::error_category *cat = logger_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(logger_error e) noexcept {
    return {static_cast<int>(e), logger_category()};
}

}

namespace {
    const silicon::logger::logger_category_impl s_default_logger_category{};
    const bool s_logger_category_registered = [] {
        silicon::logger::inject_logger_error_category(s_default_logger_category);
        return true;
    }();
}
