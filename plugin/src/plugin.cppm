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
class IPlugin {
  public:
    virtual ~IPlugin() = default;
    virtual std::string_view name() const = 0;
    virtual bool on_load() { return true; }
    virtual bool on_unload() { return true; }
    virtual bool on_reload() { return true; }
};

/// 插件注册表
class IPluginRegistry {
  public:
    virtual ~IPluginRegistry() = default;
    virtual bool register_plugin(std::shared_ptr<IPlugin> plugin) = 0;
    virtual IPlugin *get_plugin(std::string_view name) const = 0;
    virtual bool remove_plugin(std::string_view name) = 0;
    virtual std::vector<std::string> list_plugins() const = 0;
};

// ── 默认实现 ─────────────────────────────────────────────────────

class PluginRegistry: public IPluginRegistry {

    struct P {
      public:
      std::map<std::string, std::shared_ptr<IPlugin>, std::less<>> plugins_;
    };
    std::unique_ptr<P> m_p{std::make_unique<P>()};

  public:
    bool register_plugin(std::shared_ptr<IPlugin> plugin) override;
    IPlugin *get_plugin(std::string_view name) const override;
    bool remove_plugin(std::string_view name) override;
    std::vector<std::string> list_plugins() const override;

};

/// 动态插件加载器（stub：完整实现需 shared_library + dlopen）
class IPluginLoader {
  public:
    virtual ~IPluginLoader() = default;
    virtual std::shared_ptr<IPlugin> load(const std::string &path) = 0;
};

} // namespace silicon::plugin
