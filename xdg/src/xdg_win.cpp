// 实现单元（Windows 分支）：silicon.xdg
// 仅提供平台差异辅助（env 读取/路径分隔符/home 解析/各目录默认值）；8 个导出
// API 的统一框架在 xdg.cpp（平台公共层）。与 xdg_macos.cpp / xdg_linux.cpp 的
// 守卫互斥（_WIN32 / __APPLE__ / 其余 unix），恰好一个文件定义同组辅助。
// 环境变量读取使用 _dupenv_s（MSVC 安全 CRT），避免 getenv 弃用告警。
// Windows 默认路径映射 %LOCALAPPDATA%（及 cache/state 子目录）；XDG_* 环境
// 变量如被设置则由 xdg.cpp 优先采用（符合 XDG 规范语义）。
module;

#include <cstdlib>
#include <string>

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

std::string config_home_default() { return local_app_data(); }

std::string data_home_default() { return local_app_data(); }

std::string cache_home_default() { return join(local_app_data(), "cache"); }

std::string state_home_default() { return join(local_app_data(), "state"); }

std::string config_dirs_default() { return ""; }

std::string data_dirs_default() { return ""; }

} // namespace silicon::xdg

#endif // defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
