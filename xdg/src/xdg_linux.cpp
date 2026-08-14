// 实现单元（Linux / 其余 POSIX 分支）：silicon.xdg
// 仅提供平台差异辅助；8 个导出 API 的统一框架在 xdg.cpp（平台公共层）。
// 守卫：unix 系 && !__APPLE__。语义遵循 freedesktop XDG Base Directory
// Specification：优先环境变量（由 xdg.cpp 读取），回退 $HOME 下
// .config/.local/share/.cache/.local/state 约定路径。
module;

#include <cstdlib>
#include <string>

module silicon.xdg;

#if (defined(SILICON_PLATFORM_UNIX) && !defined(SILICON_PLATFORM_APPLE))

namespace silicon::xdg {

std::string env_or(const char *name, const std::string &def) {
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : def;
}

std::string join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '/')
        return base + name;
    return base + "/" + name;
}

std::string home() {
    if(const char *e = std::getenv("HOME"); e && *e)
        return e;
    return ".";
}

std::string config_home_default() { return join(home(), ".config"); }

std::string data_home_default() { return join(home(), ".local/share"); }

std::string cache_home_default() { return join(home(), ".cache"); }

std::string state_home_default() { return join(home(), ".local/state"); }

std::string config_dirs_default() { return "/etc/xdg"; }

std::string data_dirs_default() { return "/usr/local/share:/usr/share"; }

} // namespace silicon::xdg

#endif // (defined(SILICON_PLATFORM_UNIX) && !defined(SILICON_PLATFORM_APPLE))
