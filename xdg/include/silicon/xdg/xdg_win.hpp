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

inline std::string ConfigHome() { return local_app_data(); }

inline std::string DataHome() { return local_app_data(); }

inline std::string CacheHome() { return join(local_app_data(), "cache"); }

inline std::string StateHome() { return join(local_app_data(), "state"); }

inline std::string RuntimeDir() { return ""; }

inline std::vector<std::string> ConfigDirs() { return {}; }

inline std::vector<std::string> DataDirs() { return {}; }

} // namespace silicon::xdg::detail

export namespace silicon::xdg {

inline std::string HomeDir() { return detail::home(); }

inline std::string ConfigHome() { return detail::ConfigHome(); }

inline std::string DataHome() { return detail::DataHome(); }

inline std::string CacheHome() { return detail::CacheHome(); }

inline std::string StateHome() { return detail::StateHome(); }

inline std::string RuntimeDir() { return detail::RuntimeDir(); }

inline std::vector<std::string> ConfigDirs() { return detail::ConfigDirs(); }

inline std::vector<std::string> DataDirs() { return detail::DataDirs(); }

} // namespace silicon::xdg
