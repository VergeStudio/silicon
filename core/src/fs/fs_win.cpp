module;

#include <string>

#include <silicon/common.h>

module silicon.fs;

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::fs {

class win32_file_system: public file_system_base<win32_file_system> {
  public:
    std::string normalize_text(const std::string &content) const {
        std::string out;
        out.reserve(content.size() + content.size() / 8 + 1);
        for(char c: content) {
            if(c == '\n') out.push_back('\r');
            out.push_back(c);
        }
        return out;
    }

};

SILICON_CORE_API file_system_proxy create_file_system() {
    return make_file_system<win32_file_system>();
}

}

#endif
