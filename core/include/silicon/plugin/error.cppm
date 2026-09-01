module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.plugin.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::plugin {

CORE_API std::atomic<const std::error_category *> plugin_error_category_instance{nullptr};

} // namespace silicon::plugin

export namespace silicon::plugin {

/// 插件语义错误枚举（专属 category：silicon.plugin）。
enum class plugin_error {
    kLoadFailed = 1,
    kUnloadFailed,
    kDuplicate,
    kNotFound,
    kNullPlugin,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
class CORE_API plugin_category_impl final : public std::error_category {
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

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_plugin_error_category(const std::error_category &cat) noexcept {
    plugin_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 plugin_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &plugin_category() noexcept {
    const std::error_category *cat = plugin_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// plugin_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(plugin_error e) noexcept {
    return {static_cast<int>(e), plugin_category()};
}

} // namespace silicon::plugin
