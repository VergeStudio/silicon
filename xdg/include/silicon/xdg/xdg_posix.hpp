#pragma once

// POSIX（Linux / Unix / macOS）平台实现：由 silicon.xdg 模块按编译期
// 平台检测选中后 #include。本头文件经 export namespace 把实现导出进模块。

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#if defined(__APPLE__)
#    include <pwd.h>
#    include <unistd.h>
#endif

namespace silicon::xdg::detail {

inline std::string env_or(const char *name, const std::string &def) {
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : def;
}

inline std::string home() {
    if(const char *e = std::getenv("HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    if(struct passwd *pw = getpwuid(getuid()); pw && pw->pw_dir)
        return pw->pw_dir;
#endif
    return ".";
}

inline std::string join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '/')
        return base + name;
    return base + "/" + name;
}

inline std::string config_home() {
    if(const char *e = std::getenv("XDG_CONFIG_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return join(home(), "Library/Application Support");
#else
    return join(home(), ".config");
#endif
}

inline std::string data_home() {
    if(const char *e = std::getenv("XDG_DATA_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return join(home(), "Library/Application Support");
#else
    return join(home(), ".local/share");
#endif
}

inline std::string cache_home() {
    if(const char *e = std::getenv("XDG_CACHE_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return join(home(), "Library/Caches");
#else
    return join(home(), ".cache");
#endif
}

inline std::string state_home() {
    if(const char *e = std::getenv("XDG_STATE_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return join(home(), "Library/Application Support/State");
#else
    return join(home(), ".local/state");
#endif
}

inline std::string runtime_dir() {
    if(const char *e = std::getenv("XDG_RUNTIME_DIR"); e && *e)
        return e;
    return "";
}

inline std::vector<std::string> split_paths(const char *env, const char *def) {
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

inline std::vector<std::string> config_dirs() {
    return split_paths("XDG_CONFIG_DIRS", "/etc/xdg");
}

inline std::vector<std::string> data_dirs() {
    return split_paths("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
}

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
