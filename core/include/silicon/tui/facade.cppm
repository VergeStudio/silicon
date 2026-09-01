module;

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <tuple>
// proxy dispatch 宏头：宏不随 C++20 模块导出，必须在全局模块片段文本包含
#include <silicon/proxy/proxy_macros.h>

export module silicon.tui;

// 原此处 import silicon.core；core.cppm 伞模块 re-export tui 后会形成
// core -> tui -> core 环路，且本单元实际只消费 silicon.proxy，故删除。
import silicon.proxy;

export import silicon.tui.error;

export namespace silicon::tui {

// ── 终端抽象（silicon.proxy type-erased 门面）──

PRO_DEF_MEM_DISPATCH(MemTerminalType, terminal_type);
PRO_DEF_MEM_DISPATCH(MemTerminalWidth, width);
PRO_DEF_MEM_DISPATCH(MemTerminalHeight, height);
struct terminal_facade : silicon::proxy::facade_builder
    ::add_convention<MemTerminalType, std::string_view() const>
    ::add_convention<MemTerminalWidth, int32_t() const>
    ::add_convention<MemTerminalHeight, int32_t() const>::build {};

using terminal_proxy = silicon::proxy::proxy<terminal_facade>;
using terminal_view = silicon::proxy::proxy_view<terminal_facade>;

template <class T, class... Args>
[[nodiscard]] terminal_proxy make_terminal(Args &&...args) {
    return silicon::proxy::make_proxy<terminal_facade, T>(
        std::forward<Args>(args)...);
}

template <class T>
    requires silicon::proxy::proxiable_target<T, terminal_facade>
[[nodiscard]] terminal_view make_terminal_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<terminal_facade>(target);
}

#if defined(SILICON_PLATFORM_UNIX)

class unix_terminal {
  public:
    std::string_view terminal_type() const;
    int32_t width() const;
    int32_t height() const;
};
#else
class default_terminal {
  public:
    std::string_view terminal_type() const;
    int32_t width() const;
    int32_t height() const;
};
#endif

// ── PTY 抽象（silicon.proxy type-erased 门面）──

PRO_DEF_MEM_DISPATCH(MemPtyCreate, create);
PRO_DEF_MEM_DISPATCH(MemPtyWrite, write);
PRO_DEF_MEM_DISPATCH(MemPtyRead, read);
PRO_DEF_MEM_DISPATCH(MemPtyClose, close);
struct pty_facade : silicon::proxy::facade_builder
    ::add_convention<MemPtyCreate,
                     bool(std::string_view, std::initializer_list<std::string>)>
    ::add_convention<MemPtyWrite, int32_t(std::string_view)>
    ::add_convention<MemPtyRead, std::string()>
    ::add_convention<MemPtyClose, void()>::build {};

using pty_proxy = silicon::proxy::proxy<pty_facade>;
using pty_view = silicon::proxy::proxy_view<pty_facade>;

template <class T, class... Args>
[[nodiscard]] pty_proxy make_pty(Args &&...args) {
    return silicon::proxy::make_proxy<pty_facade, T>(std::forward<Args>(args)...);
}

template <class T>
    requires silicon::proxy::proxiable_target<T, pty_facade>
[[nodiscard]] pty_view make_pty_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<pty_facade>(target);
}

// ── TUI 渲染器（silicon.proxy type-erased 门面）──

PRO_DEF_MEM_DISPATCH(MemRendererRender, render);
PRO_DEF_MEM_DISPATCH(MemRendererClear, clear);
struct tui_renderer_facade : silicon::proxy::facade_builder
    ::add_convention<MemRendererRender, void(std::string_view)>
    ::add_convention<MemRendererClear, void()>::build {};

using tui_renderer_proxy = silicon::proxy::proxy<tui_renderer_facade>;
using tui_renderer_view = silicon::proxy::proxy_view<tui_renderer_facade>;

template <class T, class... Args>
[[nodiscard]] tui_renderer_proxy make_tui_renderer(Args &&...args) {
    return silicon::proxy::make_proxy<tui_renderer_facade, T>(
        std::forward<Args>(args)...);
}

template <class T>
    requires silicon::proxy::proxiable_target<T, tui_renderer_facade>
[[nodiscard]] tui_renderer_view make_tui_renderer_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<tui_renderer_facade>(target);
}

} // namespace silicon::tui
