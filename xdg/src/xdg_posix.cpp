// 实现单元（POSIX 分支）：silicon.xdg
// 与 xdg_win.cpp 按平台二选一编译（见 xmake.lua 的 is_plat 判断），
// 避免同一模块内符号重复定义。语义遵循 freedesktop XDG Base Directory
// Specification：优先环境变量，回退 $HOME 下约定路径（macOS 回退 ~/Library）。
module;

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#if defined(__APPLE__)
#    include <pwd.h>
#    include <unistd.h>
#endif

module silicon.xdg;

namespace silicon::xdg::detail {

std::string env_or(const char *name, const std::string &def) {
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : def;
}

std::string home() {
    if(const char *e = std::getenv("HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    if(struct passwd *pw = getpwuid(getuid()); pw && pw->pw_dir)
        return pw->pw_dir;
#endif
    return ".";
}

std::string join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '/')
        return base + name;
    return base + "/" + name;
}

std::string ConfigHome() {
    if(const char *e = std::getenv("XDG_CONFIG_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return join(home(), "Library/Application Support");
#else
    return join(home(), ".config");
#endif
}

std::string DataHome() {
    if(const char *e = std::getenv("XDG_DATA_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return join(home(), "Library/Application Support");
#else
    return join(home(), ".local/share");
#endif
}

std::string CacheHome() {
    if(const char *e = std::getenv("XDG_CACHE_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return join(home(), "Library/Caches");
#else
    return join(home(), ".cache");
#endif
}

std::string StateHome() {
    if(const char *e = std::getenv("XDG_STATE_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return join(home(), "Library/Application Support/State");
#else
    return join(home(), ".local/state");
#endif
}

std::string RuntimeDir() {
    if(const char *e = std::getenv("XDG_RUNTIME_DIR"); e && *e)
        return e;
    return "";
}

std::vector<std::string> split_paths(const char *env, const char *def) {
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

std::vector<std::string> ConfigDirs() {
    return split_paths("XDG_CONFIG_DIRS", "/etc/xdg");
}

std::vector<std::string> DataDirs() {
    return split_paths("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
}

} // namespace silicon::xdg::detail

export namespace silicon::xdg {

std::string HomeDir() { return detail::home(); }

std::string ConfigHome() { return detail::ConfigHome(); }

std::string DataHome() { return detail::DataHome(); }

std::string CacheHome() { return detail::CacheHome(); }

std::string StateHome() { return detail::StateHome(); }

std::string RuntimeDir() { return detail::RuntimeDir(); }

std::vector<std::string> ConfigDirs() { return detail::ConfigDirs(); }

std::vector<std::string> DataDirs() { return detail::DataDirs(); }

} // namespace silicon::xdg
