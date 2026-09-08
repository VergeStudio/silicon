module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <cstdio>
#    include <string>
#endif

module silicon.ai.llm;

#if defined(_MSC_VER)
import silicon.ai.llm;
#endif

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::ai::llm {

// Windows 平台接缝：_popen/_pclose。POSIX 实现见 llm_unix.cpp。

std::FILE *llm_popen_read(const std::string &command) { return ::_popen(command.c_str(), "r"); }

void llm_pclose(std::FILE *pipe) { ::_pclose(pipe); }

}

#endif
