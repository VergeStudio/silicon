module;

#include <cstdint>
#include <string_view>

module silicon.tui;

#if defined(_MSC_VER)
import silicon.tui;
#endif

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::tui {

// default_terminal 的实现（接口见 :default_terminal 分区）；UNIX 端为 tui_unix.cpp。

std::string_view default_terminal::terminal_type() const { return "unknown"; }
int32_t default_terminal::width() const { return 80; }
int32_t default_terminal::height() const { return 24; }

}

#endif
