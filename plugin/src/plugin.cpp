module;

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

module silicon.plugin;

namespace silicon::plugin {

bool PluginRegistry::register_plugin(std::shared_ptr<IPlugin> plugin) {
    if(!plugin) return false;
    auto name = std::string(plugin->name());
    if(m_p->plugins_.contains(name)) return false;
    m_p->plugins_[std::move(name)] = std::move(plugin);
    return true;
}

IPlugin *PluginRegistry::get_plugin(std::string_view name) const {
    auto it = m_p->plugins_.find(name);
    return (it != m_p->plugins_.end()) ? it->second.get() : nullptr;
}

bool PluginRegistry::remove_plugin(std::string_view name) {
    auto it = m_p->plugins_.find(name);
    if(it == m_p->plugins_.end()) return false;
    it->second->on_unload();
    m_p->plugins_.erase(it);
    return true;
}

std::vector<std::string> PluginRegistry::list_plugins() const {
    std::vector<std::string> names;
    for(const auto &[k, v]: m_p->plugins_) names.push_back(k);
    return names;
}

} // namespace silicon::plugin
