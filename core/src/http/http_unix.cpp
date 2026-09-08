module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <cstdio>
#    include <string>
#endif

module silicon.http;

#if defined(_MSC_VER)
import silicon.http;
#endif

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::http {

// POSIX 平台接缝：popen/pclose。Windows 实现见 http_win.cpp。

std::FILE *http_popen_read(const std::string &command) { return ::popen(command.c_str(), "r"); }

void http_pclose(std::FILE *pipe) { ::pclose(pipe); }

}

#endif
