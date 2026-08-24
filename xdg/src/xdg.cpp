// 实现单元（平台公共层）：silicon.xdg
// 各平台相同的部分统一于此：
//   - 8 个导出 API 的定义（统一框架：XDG_* 环境变量优先，回退平台默认路径）
//   - 平台无关辅助 split_paths（':' 分割路径列表）
// 平台差异（env 读取方式、路径分隔符、home 解析、各目录默认路径）由
// xdg_win.cpp / xdg_macos.cpp / xdg_linux.cpp 提供（守卫互斥，恰好一个文件
// 定义同组辅助），本文件以同模块前向声明引用之——实现单元内不得出现 export，
// 导出性由接口单元 xdg.cppm 的声明决定。
module;

#include <sstream>
#include <string>
#include <vector>

module silicon.xdg;

namespace silicon::xdg {

// ── 平台辅助前向声明（定义在各平台实现单元，守卫互斥） ──────────────
std::string env_or(const char *, const std::string &);
std::string join(const std::string &, const std::string &);
std::string home();

std::string config_home_default();
std::string data_home_default();
std::string cache_home_default();
std::string state_home_default();
std::string config_dirs_default();
std::string data_dirs_default();

// ── 平台无关辅助 ─────────────────────────────────────────────────
std::vector<std::string> split_paths(const char *env, const std::string &def) {
    std::vector<std::string> out;
    std::string s = env_or(env, def);
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, ':')) {
        if(!item.empty())
            out.push_back(item);
    }
    return out;
}

// ── 导出 API 定义（接口单元 xdg.cppm 已声明） ──────────────────────
std::string home_dir() { return home(); }

std::string config_home() { return env_or("XDG_CONFIG_HOME", config_home_default()); }

std::string data_home() { return env_or("XDG_DATA_HOME", data_home_default()); }

std::string cache_home() { return env_or("XDG_CACHE_HOME", cache_home_default()); }

std::string state_home() { return env_or("XDG_STATE_HOME", state_home_default()); }

std::string runtime_dir() { return env_or("XDG_RUNTIME_DIR", ""); }

std::vector<std::string> config_dirs() { return split_paths("XDG_CONFIG_DIRS", config_dirs_default()); }

std::vector<std::string> data_dirs() { return split_paths("XDG_DATA_DIRS", data_dirs_default()); }

} // namespace silicon::xdg
