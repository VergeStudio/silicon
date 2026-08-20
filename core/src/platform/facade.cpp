module;

#include <string>

module silicon.platform;

class windows_platform {
  public:
    std::string os_name() const { return "windows"; }
    char path_separator() const { return '\\'; }
    std::string line_ending() const { return "\r\n"; }
};

class linux_platform {
  public:
    std::string os_name() const { return "linux"; }
    char path_separator() const { return '/'; }
    std::string line_ending() const { return "\n"; }
};

class unix_platform {
  public:
    std::string os_name() const { return "unix"; }
    char path_separator() const { return '/'; }
    std::string line_ending() const { return "\n"; }
};

// 编译期选中当前平台实现（互斥，仅一个分支参与重载决议），返回拥有句柄。
platform_proxy create_platform() {
    if constexpr(os == os_id::kWindowsNt) {
        return make_platform<windows_platform>();
    } else if constexpr(os == os_id::kLinuxOs) {
        return make_platform<linux_platform>();
    } else {
        return make_platform<unix_platform>();
    }
}
