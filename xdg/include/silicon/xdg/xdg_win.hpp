#pragma once

// Windows 平台实现：由 silicon.xdg 模块按编译期平台检测选中后 #include。
// 本头文件经 export namespace 把实现导出进模块。
// 环境变量读取使用 _dupenv_s（MSVC 安全 CRT），避免 getenv 弃用告警。

#include <cstdlib>
#include <string>
#include <vector>

namespace silicon::xdg::detail {

inline std::string env_or(const char *name, const std::string &def) {
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

inline std::string home() {
    return env_or("USERPROFILE", ".");
}

inline std::string join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '\\')
        return base + name;
    return base + "\\" + name;
}

inline std::string local_app_data() {
    return env_or("LOCALAPPDATA", join(home(), "AppData\\Local"));
}

inline std::string config_home() { return local_app_data(); }

inline std::string data_home() { return local_app_data(); }

inline std::string cache_home() { return join(local_app_data(), "cache"); }

inline std::string state_home() { return join(local_app_data(), "state"); }

inline std::string runtime_dir() { return ""; }

inline std::vector<std::string> config_dirs() { return {}; }

inline std::vector<std::string> data_dirs() { return {}; }

} // namespace silicon::xdg::detail

export namespace silicon::xdg {

inline std::string home_dir() { return detail::home(); }

inline std::string config_home() { return detail::config_home(); }

inline std::string data_home() { return detail::data_home(); }

inline std::string cache_home() { return detail::cache_home(); }

inline std::string state_home() { return detail::state_home(); }

inline std::string runtime_dir() { return detail::runtime_dir(); }

inline std::vector<std::string> config_dirs() { return detail::config_dirs(); }

inline std::vector<std::string> data_dirs() { return detail::data_dirs(); }

} // namespace silicon::xdg
