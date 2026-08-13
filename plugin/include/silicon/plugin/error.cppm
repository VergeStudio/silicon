module;

#include <string>
#include <system_error>

export module silicon.plugin.error;

export namespace silicon::plugin {

/// 插件语义错误枚举（专属 category：silicon.plugin）。
enum class plugin_error {
    kLoadFailed = 1,
    kUnloadFailed,
    kDuplicate,
    kNotFound,
    kNullPlugin,
};

/// 返回 plugin_error 专属 error_category（name() == "silicon.plugin"）。
[[nodiscard]] inline const std::error_category &plugin_category() noexcept {
    static const class : public std::error_category {
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
    } cat;
    return cat;
}

/// plugin_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(plugin_error e) noexcept {
    return {static_cast<int>(e), plugin_category()};
}

} // namespace silicon::plugin
