module;

#include <cstdint>
#include <string_view>

#include <silicon/common.h>
export module silicon.tui:default_terminal;

// 默认终端实现（固定 80x24 的退化实现）。
// 仅在 Windows 平台有内容，其余平台为空分区；UNIX 端为 unix_terminal 分区。

#if defined(SILICON_PLATFORM_WINDOWS)

export namespace silicon::tui {

class SILICON_CORE_API default_terminal {
  public:
    std::string_view terminal_type() const;
    int32_t width() const;
    int32_t height() const;
};

}

#endif
