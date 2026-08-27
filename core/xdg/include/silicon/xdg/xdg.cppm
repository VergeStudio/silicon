/// @file xdg.cppm
/// @brief XDG Base Directory 规范的跨平台实现。
/// @usage
///   import silicon.xdg;
///   auto cfg = silicon::xdg::config_home();   // 用户配置目录
///
/// 语义遵循 freedesktop XDG Base Directory Specification：
///   - 优先读取对应环境变量（XDG_CONFIG_HOME 等）
///   - 未设置则回退到各平台约定路径
///   - POSIX 回退到 $HOME 下的 .config/.local/share/...（macOS 回退到 ~/Library）
///   - Windows 映射到 %LOCALAPPDATA%（及 cache/state 子目录）

module;

#include <string>
#include <vector>
#include <silicon/xdg/common.h>

export module silicon.xdg;

export namespace silicon::xdg {

/// 当前用户主目录（POSIX: $HOME / getpwuid；Windows: %USERPROFILE%）。
XDG_API std::string home_dir();

/// XDG_DATA_HOME：用户级数据目录。
XDG_API std::string data_home();

/// XDG_CONFIG_HOME：用户级配置目录。
XDG_API std::string config_home();

/// XDG_CACHE_HOME：用户级缓存目录。
XDG_API std::string cache_home();

/// XDG_STATE_HOME：用户级状态目录。
XDG_API std::string state_home();

/// XDG_RUNTIME_DIR：用户级运行时目录（无默认值则返回空串）。
XDG_API std::string runtime_dir();

/// XDG_DATA_DIRS：系统级数据目录集合（有序）。
XDG_API std::vector<std::string> data_dirs();

/// XDG_CONFIG_DIRS：系统级配置目录集合（有序）。
XDG_API std::vector<std::string> config_dirs();

} // namespace silicon::xdg

// ── 分平台实现（home_dir / *Home / *Dirs 等） ──────────────────────────
// 此前由 xdg_win.hpp / xdg_posix.hpp 经 #include 注入本接口单元；现统一
// 移入模块实现单元 src/xdg.cpp，按编译器预定义宏在编译期选用对应平台分支，
// 使模块自包含、不依赖消费项目的宏定义。
