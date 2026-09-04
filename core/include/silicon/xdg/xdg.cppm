











module;

#include <string>
#include <vector>
#include <silicon/common.h>

export module silicon.xdg;

export namespace silicon::xdg {


CORE_API std::string home_dir();


CORE_API std::string data_home();


CORE_API std::string config_home();


CORE_API std::string cache_home();


CORE_API std::string state_home();


CORE_API std::string runtime_dir();


CORE_API std::vector<std::string> data_dirs();


CORE_API std::vector<std::string> config_dirs();

}



