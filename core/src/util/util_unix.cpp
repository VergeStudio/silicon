module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <cstdlib>
#    include <string>
#endif

module silicon.util;

#if defined(_MSC_VER)
import silicon.util;
#endif

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::os {

// POSIX 平台接缝：环境变量读写。Windows 实现见 util_win.cpp。

std::string get_env(const char *name) {
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : std::string{};
}

void set_env(const char *name, const char *value) {
    setenv(name, value, 1);
}

}

#endif
