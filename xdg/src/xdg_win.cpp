// 实现单元（Windows 分支）：silicon.xdg
// 与 xdg_posix.cpp 均被 xmake 收集编译，平台选择由本文件内的 #if 守卫完成：
// 非 Windows 平台下本文件内容为空（仅模块声明），实体仅存在于对应当前平台的
// 那个文件，避免同一模块内符号重复定义。环境变量读取使用 _dupenv_s
// （MSVC 安全 CRT），避免 getenv 弃用告警。
module;

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

module silicon.xdg;

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

namespace silicon::xdg::detail {

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

std::string home() {
    return env_or("USERPROFILE", ".");
}

std::string join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '\\')
        return base + name;
    return base + "\\" + name;
}

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

#endif // defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
