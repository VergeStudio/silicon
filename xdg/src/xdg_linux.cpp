// 实现单元（Linux / 其余 POSIX 分支）：silicon.xdg
// 与 xdg_win.cpp / xdg_macos.cpp 均被 xmake 收集编译，平台选择由本文件内的
// #if 守卫完成：非 unix 系（或 macOS）平台下本文件内容为空（仅模块声明），
// 实体仅存在于对应当前平台的那个文件，避免同一模块内符号重复定义。
// 语义遵循 freedesktop XDG Base Directory Specification：优先环境变量，
// 回退 $HOME 下 .config/.local/share/.cache/.local/state 约定路径。
// 共享 POSIX 辅助（impl_env_or/impl_join/impl_split_paths 等）随平台文件各持一份副本。
module;

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

module silicon.xdg;

#if (defined(__unix__) || defined(__unix) || defined(unix)) && !defined(__APPLE__)

namespace silicon::xdg {

std::string impl_env_or(const char *name, const std::string &def) {
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : def;
}

std::string impl_home() {
    if(const char *e = std::getenv("HOME"); e && *e)
        return e;
    return ".";
}

std::string impl_join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '/')
        return base + name;
    return base + "/" + name;
}

std::string impl_ConfigHome() {
    if(const char *e = std::getenv("XDG_CONFIG_HOME"); e && *e)
        return e;
    return impl_join(impl_home(), ".config");
}

std::string impl_DataHome() {
    if(const char *e = std::getenv("XDG_DATA_HOME"); e && *e)
        return e;
    return impl_join(impl_home(), ".local/share");
}

std::string impl_CacheHome() {
    if(const char *e = std::getenv("XDG_CACHE_HOME"); e && *e)
        return e;
    return impl_join(impl_home(), ".cache");
}

std::string impl_StateHome() {
    if(const char *e = std::getenv("XDG_STATE_HOME"); e && *e)
        return e;
    return impl_join(impl_home(), ".local/state");
}

std::string impl_RuntimeDir() {
    if(const char *e = std::getenv("XDG_RUNTIME_DIR"); e && *e)
        return e;
    return "";
}

std::vector<std::string> impl_split_paths(const char *env, const char *def) {
    std::vector<std::string> out;
    std::string s = impl_env_or(env, def);
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, ':')) {
        if(!item.empty())
            out.push_back(item);
    }
    return out;
}

std::vector<std::string> impl_ConfigDirs() {
    return impl_split_paths("XDG_CONFIG_DIRS", "/etc/xdg");
}

std::vector<std::string> impl_DataDirs() {
    return impl_split_paths("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
}

} // namespace silicon::xdg

export namespace silicon::xdg {

std::string home_dir() { return impl_home(); }

std::string config_home() { return impl_ConfigHome(); }

std::string data_home() { return impl_DataHome(); }

std::string cache_home() { return impl_CacheHome(); }

std::string state_home() { return impl_StateHome(); }

std::string runtime_dir() { return impl_RuntimeDir(); }

std::vector<std::string> config_dirs() { return impl_ConfigDirs(); }

std::vector<std::string> data_dirs() { return impl_DataDirs(); }

} // namespace silicon::xdg

#endif // (defined(__unix__) || defined(__unix) || defined(unix)) && !defined(__APPLE__)
