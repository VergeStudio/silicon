module;

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module silicon.plugin;

export namespace silicon::plugin {

/// 插件生命周期
class Plugin {
  public:
    virtual ~Plugin() = default;
    virtual std::string_view Name() const = 0;
    virtual bool OnLoad() { return true; }
    virtual bool OnUnload() { return true; }
    virtual bool OnReload() { return true; }
};

/// 插件注册表
class PluginRegistry {
  public:
    virtual ~PluginRegistry() = default;
    virtual bool RegisterPlugin(std::shared_ptr<Plugin> plugin) = 0;
    virtual Plugin *GetPlugin(std::string_view name) const = 0;
    virtual bool RemovePlugin(std::string_view name) = 0;
    virtual std::vector<std::string> ListPlugins() const = 0;
};

// ── 默认实现 ─────────────────────────────────────────────────────

class DefaultPluginRegistry: public PluginRegistry {

    struct Impl {
      public:
      std::map<std::string, std::shared_ptr<Plugin>, std::less<>> plugins_;
    };
    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};

  public:
    bool RegisterPlugin(std::shared_ptr<Plugin> plugin) override;
    Plugin *GetPlugin(std::string_view name) const override;
    bool RemovePlugin(std::string_view name) override;
    std::vector<std::string> ListPlugins() const override;

};

/// 动态插件加载器（stub：完整实现需 shared_library + dlopen）
class PluginLoader {
  public:
    virtual ~PluginLoader() = default;
    virtual std::shared_ptr<Plugin> Load(const std::string &path) = 0;
};

} // namespace silicon::plugin
