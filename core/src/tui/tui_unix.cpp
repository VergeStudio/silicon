module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <sys/ioctl.h>
#    include <unistd.h>
#endif

#include <memory>
#include <string>
#include <string_view>

module silicon.tui;

#if defined(_MSC_VER)
import silicon.tui;
#endif

import silicon.util;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::tui {

// unix_terminal 的实现（接口见 :unix_terminal 分区）；Windows 端为 tui_win.cpp。

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

}

#endif
