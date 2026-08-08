/// @file xdg.cppm
/// @brief XDG Base Directory 规范的跨平台实现。
/// @usage
///   import silicon.xdg;
///   auto cfg = silicon::xdg::ConfigHome();   // 用户配置目录
///
/// 语义遵循 freedesktop XDG Base Directory Specification：
///   - 优先读取对应环境变量（XDG_CONFIG_HOME 等）
///   - 未设置则回退到各平台约定路径
///   - POSIX 回退到 $HOME 下的 .config/.local/share/...（macOS 回退到 ~/Library）
///   - Windows 映射到 %LOCALAPPDATA%（及 cache/state 子目录）

module;

#include <string>
#include <vector>

export module silicon.xdg;

export namespace silicon::xdg {

/// 当前用户主目录（POSIX: $HOME / getpwuid；Windows: %USERPROFILE%）。
std::string HomeDir();

/// XDG_DATA_HOME：用户级数据目录。
std::string DataHome();

/// XDG_CONFIG_HOME：用户级配置目录。
std::string ConfigHome();

/// XDG_CACHE_HOME：用户级缓存目录。
std::string CacheHome();

/// XDG_STATE_HOME：用户级状态目录。
std::string StateHome();

/// XDG_RUNTIME_DIR：用户级运行时目录（无默认值则返回空串）。
std::string RuntimeDir();

/// XDG_DATA_DIRS：系统级数据目录集合（有序）。
std::vector<std::string> DataDirs();

/// XDG_CONFIG_DIRS：系统级配置目录集合（有序）。
std::vector<std::string> ConfigDirs();

} // namespace silicon::xdg

// ── 分平台实现（HomeDir / *Home / *Dirs 等） ──────────────────────────
// 此前由 xdg_win.hpp / xdg_posix.hpp 经 #include 注入本接口单元；现统一
// 移入模块实现单元 src/xdg.cpp，按编译器预定义宏在编译期选用对应平台分支，
// 使模块自包含、不依赖消费项目的宏定义。
