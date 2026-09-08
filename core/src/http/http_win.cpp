module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <cstdio>
#    include <string>
#endif

module silicon.http;

#if defined(_MSC_VER)
import silicon.http;
#endif

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::http {

// Windows 平台接缝：_popen/_pclose。POSIX 实现见 http_unix.cpp。

std::FILE *http_popen_read(const std::string &command) { return ::_popen(command.c_str(), "r"); }

void http_pclose(std::FILE *pipe) { ::_pclose(pipe); }

}

#endif
