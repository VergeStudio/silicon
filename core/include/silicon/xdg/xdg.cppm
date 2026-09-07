module;

#include <string>
#include <vector>
#include <silicon/common.h>

export module silicon.xdg;

export namespace silicon::xdg {

SILICON_CORE_API std::string home_dir();

SILICON_CORE_API std::string data_home();

SILICON_CORE_API std::string config_home();

SILICON_CORE_API std::string cache_home();

SILICON_CORE_API std::string state_home();

SILICON_CORE_API std::string runtime_dir();

SILICON_CORE_API std::vector<std::string> data_dirs();

SILICON_CORE_API std::vector<std::string> config_dirs();

}
