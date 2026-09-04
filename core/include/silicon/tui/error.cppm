module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.tui.error;

import silicon.error;

namespace silicon::tui {

CORE_API std::atomic<const std::error_category *> tui_error_category_instance{nullptr};

}

export namespace silicon::tui {

enum class tui_error {
    kInitFailed = 1,
    kInvalidTerminal,
};

class CORE_API tui_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.tui"; }
    std::string message(int ev) const override {
        switch(static_cast<tui_error>(ev)) {
            case tui_error::kInitFailed: return "tui init failed";
            case tui_error::kInvalidTerminal: return "invalid terminal";
        }
        return "unknown tui error";
    }
};

inline void inject_tui_error_category(const std::error_category &cat) noexcept {
    tui_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &tui_category() noexcept {
    const std::error_category *cat = tui_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(tui_error e) noexcept {
    return {static_cast<int>(e), tui_category()};
}

}

namespace {
    const silicon::tui::tui_category_impl s_default_tui_category{};
    const bool s_tui_category_registered = [] {
        silicon::tui::inject_tui_error_category(s_default_tui_category);
        return true;
    }();
}
