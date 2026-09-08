module;

#include <filesystem>
#include <string>
#include <system_error>

#include <silicon/common.h>

module silicon.fs;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::fs {

class posix_file_system: public file_system_base<posix_file_system> {
  public:
    bool create_directories(const std::string &path) const {
        std::error_code ec;
        auto p = to_path(path);
        bool made = std::filesystem::create_directories(p, ec);
        if(ec) return false;
        std::filesystem::permissions(p, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec, std::filesystem::perm_options::replace, ec);
        return made;
    }

};

SILICON_CORE_API file_system_proxy create_file_system() {
    return make_file_system<posix_file_system>();
}

}

#endif
