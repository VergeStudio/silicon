module;

#include <memory>
#include <string>
#include <string_view>

#if defined(SILICON_PLATFORM_UNIX)
#    include <sys/ioctl.h>
#    include <unistd.h>
#endif

module silicon.tui;

// 直接依赖 silicon.util（silicon::os::get_env 声明于此；silicon.core 伞模块
// re-export 了 tui，此处 import core 会形成 core -> tui -> core 环路）。
import silicon.util;

namespace silicon::tui {

#if defined(SILICON_PLATFORM_UNIX)

std::string_view unix_terminal::terminal_type() const {
    static std::string t = [] {
        std::string v = silicon::os::get_env("TERM");
        return v.empty() ? "xterm-256color" : v;
    }();
    return t;
}

int32_t unix_terminal::width() const {
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) return w.ws_col;
    return 80;
}

int32_t unix_terminal::height() const {
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) return w.ws_row;
    return 24;
}

#else

std::string_view default_terminal::terminal_type() const { return "unknown"; }
int32_t default_terminal::width() const { return 80; }
int32_t default_terminal::height() const { return 24; }

#endif

} // namespace silicon::tui
