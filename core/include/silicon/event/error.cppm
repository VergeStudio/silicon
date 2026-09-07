module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>
#include <silicon/common.h>

export module silicon.event.error;

import silicon.error;

export namespace silicon::event {

SILICON_CORE_API std::atomic<const std::error_category *> event_error_category_instance{nullptr};

enum class event_error {
    kInvalidStatus = 1,
};

class SILICON_CORE_API event_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.event"; }
    std::string message(int ev) const override {
        switch(static_cast<event_error>(ev)) {
            case event_error::kInvalidStatus: return "invalid event status";
        }
        return "unknown event error";
    }
};

inline void inject_event_error_category(const std::error_category &cat) noexcept {
    event_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &event_category() noexcept {
    const std::error_category *cat = event_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(event_error e) noexcept {
    return {static_cast<int>(e), event_category()};
}

}

namespace {
    const silicon::event::event_category_impl s_default_event_category{};
    const bool s_event_category_registered = [] {
        silicon::event::inject_event_error_category(s_default_event_category);
        return true;
    }();
}
