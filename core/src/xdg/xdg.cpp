module;

#include <sstream>
#include <string>
#include <vector>
#include <silicon/common.h>

module silicon.xdg;

namespace silicon::xdg {

std::string env_or(const char *, const std::string &);
std::string join(const std::string &, const std::string &);
std::string home();

std::string config_home_default();
std::string data_home_default();
std::string cache_home_default();
std::string state_home_default();
std::string config_dirs_default();
std::string data_dirs_default();

std::vector<std::string> split_paths(const char *env, const std::string &def) {
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

CORE_API std::string home_dir() { return home(); }

CORE_API std::string config_home() { return env_or("XDG_CONFIG_HOME", config_home_default()); }

CORE_API std::string data_home() { return env_or("XDG_DATA_HOME", data_home_default()); }

CORE_API std::string cache_home() { return env_or("XDG_CACHE_HOME", cache_home_default()); }

CORE_API std::string state_home() { return env_or("XDG_STATE_HOME", state_home_default()); }

CORE_API std::string runtime_dir() { return env_or("XDG_RUNTIME_DIR", ""); }

CORE_API std::vector<std::string> config_dirs() { return split_paths("XDG_CONFIG_DIRS", config_dirs_default()); }

CORE_API std::vector<std::string> data_dirs() { return split_paths("XDG_DATA_DIRS", data_dirs_default()); }

}
