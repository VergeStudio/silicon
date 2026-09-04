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
import silicon.plugin.error;

namespace silicon::plugin {

auto plugin_registry::register_plugin(plugin_proxy plugin) -> result<void> {
    if(!plugin) return std::unexpected(make_error_code(plugin_error::kNullPlugin));
    auto name = std::string(plugin->name());
    if(impl_->plugins_.contains(name)) return std::unexpected(make_error_code(plugin_error::kDuplicate));
    impl_->plugins_[std::move(name)] = std::move(plugin);
    return {};
}

plugin_proxy *plugin_registry::get_plugin(std::string_view name) const {
    auto it = impl_->plugins_.find(name);
    return (it != impl_->plugins_.end()) ? std::addressof(it->second) : nullptr;
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

}
