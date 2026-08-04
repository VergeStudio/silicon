module;

#include <memory>
#include <string>
#include <string_view>

#if defined(SILICON_PLATFORM_UNIX)
#    include <sys/ioctl.h>
#    include <unistd.h>
#endif

module silicon.tui;

import silicon.core;

namespace silicon::tui {

#if defined(SILICON_PLATFORM_UNIX)

std::string_view UnixTerminal::terminal_type() const {
    static std::string t = [] {
        std::string v = silicon::os::GetEnv("TERM");
        return v.empty() ? "xterm-256color" : v;
    }();
    return t;
}

int32_t UnixTerminal::width() const {
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) return w.ws_col;
    return 80;
}

int32_t UnixTerminal::height() const {
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) return w.ws_row;
    return 24;
}

#else

std::string_view DefaultTerminal::terminal_type() const { return "unknown"; }
int32_t DefaultTerminal::width() const { return 80; }
int32_t DefaultTerminal::height() const { return 24; }

#endif

} // namespace silicon::tui
