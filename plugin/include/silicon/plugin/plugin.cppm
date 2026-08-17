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

// proxy 的 dispatch 宏定义在头文件里：宏不随 C++20 模块导出，
// 消费方必须在全局模块片段显式包含，随后再 `import silicon.proxy`。
#include <silicon/proxy/proxy_macros.h>

export module silicon.plugin;
export import silicon.plugin.error;

import silicon.proxy;

export namespace silicon::plugin {

/// 统一错误返回类型：plugin 模块所有可失败 API 返回 plugin::result<T>。
template<typename T>
using result = std::expected<T, std::error_code>;


/// 插件生命周期
///
/// 插件生命周期由 plugin_facade 以类型擦除描述（name / on_load / on_unload /
/// on_reload）。任意满足该门面的类型（含既有的 shared_ptr<X>，只要 X 具约定成员）
/// 皆可擦除为 plugin_proxy / plugin_view，无需继承任何基类。

/// 经典注册表（基于 proxy 句柄）。与 proxy_plugin_registry 并存：
/// 前者保留 shared_ptr 风格的 `name()` 查询 API（register_plugin/get_plugin/
/// remove_plugin/list_plugins），后者提供 emplace 就地构造与 `get()` 返回
/// 句柄指针。两者底层均以 plugin_proxy 值持有，跨 DLL / ABI 边界安全。
class plugin_registry {

    struct impl {
      public:
      std::map<std::string, plugin_proxy, std::less<>> plugins_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:
    /// 注册已擦除的插件；句柄为空返回 kNullPlugin，名称重复返回 kDuplicate。
    [[nodiscard]] auto register_plugin(plugin_proxy plugin) -> result<void>;

    /// 就地构造并注册；等价于 register_plugin(make_plugin<T>(args...))。
    template<class T, class... Args>
    [[nodiscard]] auto emplace(Args &&...args) -> result<void> {
        return register_plugin(make_plugin<T>(std::forward<Args>(args)...));
    }

    /// 查询；不存在返回 nullptr。返回句柄的所有权仍属注册表。
    plugin_proxy *get_plugin(std::string_view name) const;

    /// 移除并触发 on_unload；不存在返回 kNotFound。
    [[nodiscard]] auto remove_plugin(std::string_view name) -> result<void>;

    std::vector<std::string> list_plugins() const;

};

// ── 类型擦除接入层（silicon.proxy）────────────────────────────────
//
// 插件是天然的跨 DLL / ABI 边界：宿主与插件常由不同编译单元、甚至不同
// 编译器版本产出，虚表布局一旦变化即不兼容。proxy 用「胖指针 + vtable
// 值」替代继承，目标类型无需继承任何基类，也不共享 RTTI/虚表，因此更适
// 合该边界。以下设施为唯一的接口范式。

/// 成员派发器：把 `.name()` / `.on_load()` 等调用擦除为 proxy 约定。
PRO_DEF_MEM_DISPATCH(MemPluginName, name);
PRO_DEF_MEM_DISPATCH(MemPluginOnLoad, on_load);
PRO_DEF_MEM_DISPATCH(MemPluginOnUnload, on_unload);
PRO_DEF_MEM_DISPATCH(MemPluginOnReload, on_reload);

/// 插件门面：任何具备下列成员的类型都自动满足，无需继承任何基类。
///   std::string_view name() const;
///   bool on_load();  bool on_unload();  bool on_reload();
/// 既有的具约定成员的类型（含 `std::shared_ptr<X>`）亦天然满足，可直接桥接。
struct plugin_facade
    : silicon::proxy::facade_builder                                        //
      ::add_convention<MemPluginName, std::string_view() const>             //
      ::add_convention<MemPluginOnLoad, bool()>                             //
      ::add_convention<MemPluginOnUnload, bool()>                           //
      ::add_convention<MemPluginOnReload, bool()>                           //
      ::build {};

/// 拥有所有权的类型擦除插件句柄（值语义；小对象内联，无堆分配）。
/// 用法与指针一致：`p->name()`、`if (p) ...`。
using plugin_proxy = silicon::proxy::proxy<plugin_facade>;

/// 非拥有观察视图，等价于裸指针但不要求继承。
using plugin_view = silicon::proxy::proxy_view<plugin_facade>;

/// 就地构造任意满足 plugin_facade 的目标类型并擦除为 plugin_proxy。
template<class T, class... Args>
[[nodiscard]] plugin_proxy make_plugin(Args &&...args) {
    return silicon::proxy::make_proxy<plugin_facade, T>(std::forward<Args>(args)...);
}

/// 为已存在的对象创建非拥有视图；调用方负责保证生命周期。
template<class T>
    requires silicon::proxy::proxiable_target<T, plugin_facade>
[[nodiscard]] plugin_view make_plugin_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<plugin_facade>(target);
}

/// 基于 proxy 的插件注册表。
/// 与 plugin_registry 的差异：目标类型无需继承任何基类，也无需 shared_ptr —
/// 只要满足 plugin_facade 即可注册，句柄按值持有。
class proxy_plugin_registry {

    struct impl {
      public:
        std::map<std::string, plugin_proxy, std::less<>> plugins_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:
    /// 注册已擦除的插件；句柄为空返回 kNullPlugin，名称重复返回 kDuplicate。
    [[nodiscard]] auto register_plugin(plugin_proxy plugin) -> result<void>;

    /// 就地构造并注册；等价于 register_plugin(make_plugin<T>(args...))。
    template<class T, class... Args>
    [[nodiscard]] auto emplace(Args &&...args) -> result<void> {
        return register_plugin(make_plugin<T>(std::forward<Args>(args)...));
    }

    /// 查询；不存在返回 nullptr。返回句柄的所有权仍属注册表。
    plugin_proxy *get(std::string_view name) const;

    /// 移除并触发 on_unload；不存在返回 kNotFound。
    [[nodiscard]] auto remove(std::string_view name) -> result<void>;

    std::vector<std::string> list() const;

};

} // namespace silicon::plugin
