module;

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <expected>

module silicon.plugin;

namespace silicon::plugin {

auto plugin_registry::register_plugin(std::shared_ptr<i_plugin> plugin) -> result<void> {
    if(!plugin) return std::unexpected(make_error_code(plugin_error::kNullPlugin));
    auto name = std::string(plugin->name());
    if(impl_->plugins_.contains(name)) return std::unexpected(make_error_code(plugin_error::kDuplicate));
    impl_->plugins_[std::move(name)] = std::move(plugin);
    return {};
}

i_plugin *plugin_registry::get_plugin(std::string_view name) const {
    auto it = impl_->plugins_.find(name);
    return (it != impl_->plugins_.end()) ? it->second.get() : nullptr;
}

auto plugin_registry::remove_plugin(std::string_view name) -> result<void> {
    auto it = impl_->plugins_.find(name);
    if(it == impl_->plugins_.end()) return std::unexpected(make_error_code(plugin_error::kNotFound));
    it->second->on_unload();
    impl_->plugins_.erase(it);
    return {};
}

std::vector<std::string> plugin_registry::list_plugins() const {
    std::vector<std::string> names;
    for(const auto &[k, v]: impl_->plugins_) names.push_back(k);
    return names;
}

// ── proxy_plugin_registry ──────────────────────────────────────────

auto proxy_plugin_registry::register_plugin(plugin_proxy plugin) -> result<void> {
    if(!plugin) return std::unexpected(make_error_code(plugin_error::kNullPlugin));
    auto name = std::string(plugin->name());
    if(impl_->plugins_.contains(name)) return std::unexpected(make_error_code(plugin_error::kDuplicate));
    impl_->plugins_.emplace(std::move(name), std::move(plugin));
    return {};
}

plugin_proxy *proxy_plugin_registry::get(std::string_view name) const {
    auto it = impl_->plugins_.find(name);
    return (it != impl_->plugins_.end()) ? std::addressof(it->second) : nullptr;
}

auto proxy_plugin_registry::remove(std::string_view name) -> result<void> {
    auto it = impl_->plugins_.find(name);
    if(it == impl_->plugins_.end()) return std::unexpected(make_error_code(plugin_error::kNotFound));
    it->second->on_unload();
    impl_->plugins_.erase(it);
    return {};
}

std::vector<std::string> proxy_plugin_registry::list() const {
    std::vector<std::string> names;
    names.reserve(impl_->plugins_.size());
    for(const auto &[k, v]: impl_->plugins_) names.push_back(k);
    return names;
}

// ── plugin_error category 与 make_error_code ────────────────────
namespace {
class plugin_error_category final : public std::error_category {
  public:
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
} // namespace

const std::error_category &plugin_category() noexcept {
    static const plugin_error_category cat{};
    return cat;
}

std::error_code make_error_code(plugin_error e) noexcept {
    return {static_cast<int>(e), plugin_category()};
}

} // namespace silicon::plugin
