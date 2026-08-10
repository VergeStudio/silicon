module;

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

module silicon.plugin;

namespace silicon::plugin {

bool plugin_registry::register_plugin(std::shared_ptr<i_plugin> plugin) {
    if(!plugin) return false;
    auto name = std::string(plugin->name());
    if(impl_->plugins_.contains(name)) return false;
    impl_->plugins_[std::move(name)] = std::move(plugin);
    return true;
}

i_plugin *plugin_registry::get_plugin(std::string_view name) const {
    auto it = impl_->plugins_.find(name);
    return (it != impl_->plugins_.end()) ? it->second.get() : nullptr;
}

bool plugin_registry::remove_plugin(std::string_view name) {
    auto it = impl_->plugins_.find(name);
    if(it == impl_->plugins_.end()) return false;
    it->second->on_unload();
    impl_->plugins_.erase(it);
    return true;
}

std::vector<std::string> plugin_registry::list_plugins() const {
    std::vector<std::string> names;
    for(const auto &[k, v]: impl_->plugins_) names.push_back(k);
    return names;
}

// ── proxy_plugin_registry ──────────────────────────────────────────

bool proxy_plugin_registry::register_plugin(plugin_proxy plugin) {
    if(!plugin) return false;
    auto name = std::string(plugin->name());
    if(impl_->plugins_.contains(name)) return false;
    impl_->plugins_.emplace(std::move(name), std::move(plugin));
    return true;
}

plugin_proxy *proxy_plugin_registry::get(std::string_view name) const {
    auto it = impl_->plugins_.find(name);
    return (it != impl_->plugins_.end()) ? std::addressof(it->second) : nullptr;
}

bool proxy_plugin_registry::remove(std::string_view name) {
    auto it = impl_->plugins_.find(name);
    if(it == impl_->plugins_.end()) return false;
    it->second->on_unload();
    impl_->plugins_.erase(it);
    return true;
}

std::vector<std::string> proxy_plugin_registry::list() const {
    std::vector<std::string> names;
    names.reserve(impl_->plugins_.size());
    for(const auto &[k, v]: impl_->plugins_) names.push_back(k);
    return names;
}

} // namespace silicon::plugin
