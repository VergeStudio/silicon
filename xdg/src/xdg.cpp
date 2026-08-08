// 实现单元：silicon.xdg
// 原 xdg_win.hpp / xdg_posix.hpp 的分平台实现已并入此模块实现单元，按编译器
// 预定义宏在编译期选用对应平台分支（宏选择与旧 #include 逻辑一致）。
// HomeDir / *Home / *Dirs 等导出函数此前为头文件内联定义，现改为本单元内的
// 模块外联定义（声明见 xdg.cppm），保持导出语义不变。
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
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
    char *buf = nullptr;
    size_t len = 0;
    if(_dupenv_s(&buf, &len, name) == 0 && buf) {
        std::string s(buf, len);
        free(buf);
        if(!s.empty())
            return s;
    }
    return def;
#else
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : def;
#endif
}

std::string home() {
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
    return env_or("USERPROFILE", ".");
#else
    if(const char *e = std::getenv("HOME"); e && *e)
        return e;
#    if defined(__APPLE__)
    if(struct passwd *pw = getpwuid(getuid()); pw && pw->pw_dir)
        return pw->pw_dir;
#    endif
    return ".";
#endif
}

std::string join(const std::string &base, const std::string &name) {
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
    if(base.empty())
        return name;
    if(base.back() == '\\')
        return base + name;
    return base + "\\" + name;
#else
    if(base.empty())
        return name;
    if(base.back() == '/')
        return base + name;
    return base + "/" + name;
#endif
}

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

// Windows 分支：环境变量读取使用 _dupenv_s（MSVC 安全 CRT），避免 getenv 弃用告警。
std::string local_app_data() {
    return env_or("LOCALAPPDATA", join(home(), "AppData\\Local"));
}

std::string ConfigHome() { return local_app_data(); }

std::string DataHome() { return local_app_data(); }

std::string CacheHome() { return join(local_app_data(), "cache"); }

std::string StateHome() { return join(local_app_data(), "state"); }

std::string RuntimeDir() { return ""; }

std::vector<std::string> ConfigDirs() { return {}; }

std::vector<std::string> DataDirs() { return {}; }

#else

// POSIX（Linux / Unix / macOS）分支。
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

#endif

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
