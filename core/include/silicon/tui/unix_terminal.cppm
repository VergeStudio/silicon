module;

#include <cstdint>
#include <string_view>

#include <silicon/common.h>
export module silicon.tui:unix_terminal;

// UNIX 终端实现（TERM 环境变量 + TIOCGWINSZ）。
// 仅在 UNIX 平台有内容，其余平台为空分区；Windows 端为 default_terminal 分区。

#if defined(SILICON_PLATFORM_UNIX)

export namespace silicon::tui {

class SILICON_CORE_API unix_terminal {
  public:
    std::string_view terminal_type() const;
    int32_t width() const;
    int32_t height() const;
};

}

#endif
