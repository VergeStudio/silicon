// 实现单元（POSIX 分支）：silicon.xdg
// 与 xdg_win.cpp 均被 xmake 收集编译，平台选择由本文件内的 #if 守卫完成：
// 本文件用肯定形式 #if defined(__unix__)||defined(__unix)||defined(unix)||
// defined(__APPLE__) 识别 unix 系平台（含 macOS），Windows 工具链不定义这些宏，
// 故非 unix 系平台下本文件内容为空（仅模块声明）——实体仅存在于对应当前平台
// 的那个文件，避免同一模块内符号重复定义。语义遵循 freedesktop XDG Base
// Directory Specification：优先环境变量，回退 $HOME 下约定路径
// （macOS 回退 ~/Library）。
// 内部实现函数（impl_* 前缀）与导出 API（home_dir/*Home/*Dirs）同处
// silicon::xdg 命名空间但名称不同，已消除 detail 命名空间层。
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

#if defined(__unix__) || defined(__unix) || defined(unix) || defined(__APPLE__)

namespace silicon::xdg {

std::string impl_env_or(const char *name, const std::string &def) {
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : def;
}

std::string impl_home() {
    if(const char *e = std::getenv("HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    if(struct passwd *pw = getpwuid(getuid()); pw && pw->pw_dir)
        return pw->pw_dir;
#endif
    return ".";
}

std::string impl_join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '/')
        return base + name;
    return base + "/" + name;
}

std::string impl_config_home() {
    if(const char *e = std::getenv("XDG_CONFIG_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return impl_join(impl_home(), "Library/Application Support");
#else
    return impl_join(impl_home(), ".config");
#endif
}

std::string impl_data_home() {
    if(const char *e = std::getenv("XDG_DATA_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return impl_join(impl_home(), "Library/Application Support");
#else
    return impl_join(impl_home(), ".local/share");
#endif
}

std::string impl_cache_home() {
    if(const char *e = std::getenv("XDG_CACHE_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return impl_join(impl_home(), "Library/Caches");
#else
    return impl_join(impl_home(), ".cache");
#endif
}

std::string impl_state_home() {
    if(const char *e = std::getenv("XDG_STATE_HOME"); e && *e)
        return e;
#if defined(__APPLE__)
    return impl_join(impl_home(), "Library/Application Support/State");
#else
    return impl_join(impl_home(), ".local/state");
#endif
}

std::string impl_runtime_dir() {
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

std::vector<std::string> impl_config_dirs() {
    return impl_split_paths("XDG_CONFIG_DIRS", "/etc/xdg");
}

std::vector<std::string> impl_data_dirs() {
    return impl_split_paths("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
}

} // namespace silicon::xdg

export namespace silicon::xdg {

std::string home_dir() { return impl_home(); }

std::string config_home() { return impl_config_home(); }

std::string data_home() { return impl_data_home(); }

std::string cache_home() { return impl_cache_home(); }

std::string state_home() { return impl_state_home(); }

std::string runtime_dir() { return impl_runtime_dir(); }

std::vector<std::string> config_dirs() { return impl_config_dirs(); }

std::vector<std::string> data_dirs() { return impl_data_dirs(); }

} // namespace silicon::xdg

#endif // defined(__unix__) || defined(__unix) || defined(unix) || defined(__APPLE__)
