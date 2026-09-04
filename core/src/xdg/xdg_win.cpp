






module;

#include <cstdlib>
#include <string>

module silicon.xdg;

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::xdg {

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

std::string join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '\\')
        return base + name;
    return base + "\\" + name;
}

std::string home() {
    return env_or("USERPROFILE", ".");
}

std::string local_app_data() {
    return env_or("LOCALAPPDATA", join(home(), "AppData\\Local"));
}

std::string config_home_default() { return local_app_data(); }

std::string data_home_default() { return local_app_data(); }

std::string cache_home_default() { return join(local_app_data(), "cache"); }

std::string state_home_default() { return join(local_app_data(), "state"); }

std::string config_dirs_default() { return ""; }

std::string data_dirs_default() { return ""; }

}

#endif
