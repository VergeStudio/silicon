module;

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <expected>
#include <system_error>

#include <silicon/proxy/proxy_macros.h>

#include <silicon/common.h>
export module silicon.plugin;
export import silicon.plugin.error;

import silicon.proxy;
import silicon.error;

export namespace silicon::plugin {

template<typename T>
using result = silicon::error::result<T>;

PRO_DEF_MEM_DISPATCH(MemPluginName, name);
PRO_DEF_MEM_DISPATCH(MemPluginOnLoad, on_load);
PRO_DEF_MEM_DISPATCH(MemPluginOnUnload, on_unload);
PRO_DEF_MEM_DISPATCH(MemPluginOnReload, on_reload);

struct plugin_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemPluginName, std::string_view() const>
      ::add_convention<MemPluginOnLoad, bool()>
      ::add_convention<MemPluginOnUnload, bool()>
      ::add_convention<MemPluginOnReload, bool()>
      ::build {};

using plugin_proxy = silicon::proxy::proxy<plugin_facade>;

using plugin_view = silicon::proxy::proxy_view<plugin_facade>;

template<class T, class... Args>
[[nodiscard]] plugin_proxy make_plugin(Args &&...args) {
    return silicon::proxy::make_proxy<plugin_facade, T>(std::forward<Args>(args)...);
}

template<class T>
    requires silicon::proxy::proxiable_target<T, plugin_facade>
[[nodiscard]] plugin_view make_plugin_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<plugin_facade>(target);
}

class SILICON_CORE_API plugin_registry {

    struct impl {
      public:
      std::map<std::string, plugin_proxy, std::less<>> plugins_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:

    [[nodiscard]] result<void> register_plugin(plugin_proxy) ;

    template<class T, class... Args>
    [[nodiscard]] result<void> emplace(Args &&...args) {
        return register_plugin(make_plugin<T>(std::forward<Args>(args)...));
    }

    plugin_proxy *get_plugin(std::string_view) const;

    [[nodiscard]] auto remove_plugin(std::string_view) -> result<void>;

    std::vector<std::string> list_plugins() const;

};

class SILICON_CORE_API proxy_plugin_registry {

    struct impl {
      public:
        std::map<std::string, plugin_proxy, std::less<>> plugins_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:

    [[nodiscard]] result<void> register_plugin(plugin_proxy) ;

    template<class T, class... Args>
    [[nodiscard]] result<void> emplace(Args &&...args) {
        return register_plugin(make_plugin<T>(std::forward<Args>(args)...));
    }

    plugin_proxy *get(std::string_view) const;

    [[nodiscard]] auto remove(std::string_view) -> result<void>;

    std::vector<std::string> list() const;

};

}
