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

}

#endif
