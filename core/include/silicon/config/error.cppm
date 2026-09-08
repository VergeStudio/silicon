module;

#include <atomic>
#include <expected>
#include <exception>
#include <string>
#include <system_error>
#include <silicon/common.h>

export module silicon.config.error;

import silicon.error;

export namespace silicon::config {

SILICON_CORE_API std::atomic<const std::error_category *> config_error_category_instance{nullptr};

template<typename T>

enum class config_error {
    kLoadFailed = 1,
    kParseFailed,
    kInvalidValue,
    kUnknown,
};

class SILICON_CORE_API config_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.config"; }
    std::string message(int ev) const override {
        switch(static_cast<config_error>(ev)) {
            case config_error::kLoadFailed: return "config load failed";
            case config_error::kParseFailed: return "config parse failed";
            case config_error::kInvalidValue: return "invalid config value";
            case config_error::kUnknown: return "unknown config error";
        }
        return "unknown config error";
    }
};

inline void inject_config_error_category(const std::error_category &cat) noexcept {
    config_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &config_category() noexcept {
    const std::error_category *cat = config_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(config_error e) noexcept {
    return {static_cast<int>(e), config_category()};
}

}

namespace {
    const silicon::config::config_category_impl s_default_config_category{};
    const bool s_config_category_registered = [] {
        silicon::config::inject_config_error_category(s_default_config_category);
        return true;
    }();
}
