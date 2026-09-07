module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.plugin.error;

import silicon.error;

namespace silicon::plugin {

SILICON_CORE_API std::atomic<const std::error_category *> plugin_error_category_instance{nullptr};

}

export namespace silicon::plugin {

enum class plugin_error {
    kLoadFailed = 1,
    kUnloadFailed,
    kDuplicate,
    kNotFound,
    kNullPlugin,
};

class SILICON_CORE_API plugin_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.plugin"; }
    std::string message(int ev) const override {
        switch(static_cast<plugin_error>(ev)) {
            case plugin_error::kLoadFailed: return "plugin load failed";
            case plugin_error::kUnloadFailed: return "plugin unload failed";
            case plugin_error::kDuplicate: return "plugin already registered";
            case plugin_error::kNotFound: return "plugin not found";
            case plugin_error::kNullPlugin: return "null plugin handle";
        }
        return "unknown plugin error";
    }
};

inline void inject_plugin_error_category(const std::error_category &cat) noexcept {
    plugin_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &plugin_category() noexcept {
    const std::error_category *cat = plugin_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(plugin_error e) noexcept {
    return {static_cast<int>(e), plugin_category()};
}

}
