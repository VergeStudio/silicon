module;

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

module silicon.plugin;

namespace silicon::plugin {

bool PluginRegistry::RegisterPlugin(std::shared_ptr<IPlugin> plugin) {
    if(!plugin) return false;
    auto name = std::string(plugin->Name());
    if(impl_->plugins_.contains(name)) return false;
    impl_->plugins_[std::move(name)] = std::move(plugin);
    return true;
}

IPlugin *PluginRegistry::GetPlugin(std::string_view name) const {
    auto it = impl_->plugins_.find(name);
    return (it != impl_->plugins_.end()) ? it->second.get() : nullptr;
}

bool PluginRegistry::RemovePlugin(std::string_view name) {
    auto it = impl_->plugins_.find(name);
    if(it == impl_->plugins_.end()) return false;
    it->second->OnUnload();
    impl_->plugins_.erase(it);
    return true;
}

std::vector<std::string> PluginRegistry::ListPlugins() const {
    std::vector<std::string> names;
    for(const auto &[k, v]: impl_->plugins_) names.push_back(k);
    return names;
}

} // namespace silicon::plugin
