// 实现单元（Windows 分支）：silicon.xdg
// 与 xdg_macos.cpp / xdg_linux.cpp 均被 xmake 收集编译，平台选择由本文件内的
// #if 守卫完成：非 Windows 平台下本文件内容为空（仅模块声明），实体仅存在于
// 对应当前平台的那个文件，避免同一模块内符号重复定义。环境变量读取使用
// _dupenv_s（MSVC 安全 CRT），避免 getenv 弃用告警。
// 导出函数在接口单元 xdg.cppm 中声明；本实现单元直接给出定义（实现单元内
// export 为 ill-formed，不得出现）。共享辅助（env_or/join/home/local_app_data）
// 为非导出实体，随平台文件各持一份副本。
module;

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

module silicon.xdg;

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

namespace silicon::xdg {

std::string env_or(const char *name, const std::string &def) {
    char *buf = nullptr;
    size_t len = 0;
    if(_dupenv_s(&buf, &len, name) == 0 && buf) {
        std::string s(buf, len);
        free(buf);
        if(!s.empty())
            return s;
    }
    return def;
}

std::string join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '\\')
        return base + name;
    return base + "\\" + name;
}

std::string home() {
    return env_or("USERPROFILE", ".");
}

std::string local_app_data() {
    return env_or("LOCALAPPDATA", join(home(), "AppData\\Local"));
}

// ── 导出 API 定义（接口单元 xdg.cppm 已声明） ──────────────────────
std::string home_dir() { return home(); }

std::string config_home() { return local_app_data(); }

std::string data_home() { return local_app_data(); }

std::string cache_home() { return join(local_app_data(), "cache"); }

std::string state_home() { return join(local_app_data(), "state"); }

std::string runtime_dir() { return ""; }

std::vector<std::string> config_dirs() { return {}; }

std::vector<std::string> data_dirs() { return {}; }

} // namespace silicon::xdg

#endif // defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
