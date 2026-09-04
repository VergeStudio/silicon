module;

#include <cstdlib>
#include <string>

#if defined(SILICON_PLATFORM_APPLE)
#    include <pwd.h>
#    include <unistd.h>
#endif

module silicon.xdg;

#if defined(SILICON_PLATFORM_APPLE)

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
    if(struct passwd *pw = getpwuid(getuid()); pw && pw->pw_dir)
        return pw->pw_dir;
    return ".";
}

std::string config_home_default() { return join(home(), "Library/Application Support"); }

std::string data_home_default() { return join(home(), "Library/Application Support"); }

std::string cache_home_default() { return join(home(), "Library/Caches"); }

std::string state_home_default() { return join(home(), "Library/Application Support/State"); }

std::string config_dirs_default() { return "/etc/xdg"; }

std::string data_dirs_default() { return "/usr/local/share:/usr/share"; }

}

#endif
