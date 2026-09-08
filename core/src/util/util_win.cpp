module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <cstdlib>
#    include <string>
#endif

module silicon.util;

#if defined(_MSC_VER)
import silicon.util;
#endif

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::os {

// Windows 平台接缝：环境变量读写。POSIX 实现见 util_unix.cpp。

std::string get_env(const char *name) {
    char *buf = nullptr;
    size_t len = 0;
    if(_dupenv_s(&buf, &len, name) == 0 && buf != nullptr) {
        std::string value(buf);
        std::free(buf);
        return value;
    }
    return {};
}

void set_env(const char *name, const char *value) {
    _putenv_s(name, value);
}

}

#endif
