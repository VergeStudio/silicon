module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <cstdio>
#    include <string>
#endif

module silicon.ai.llm;

#if defined(_MSC_VER)
import silicon.ai.llm;
#endif

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::ai::llm {

// POSIX 平台接缝：popen/pclose。Windows 实现见 llm_win.cpp。

std::FILE *llm_popen_read(const std::string &command) { return ::popen(command.c_str(), "r"); }

void llm_pclose(std::FILE *pipe) { ::pclose(pipe); }

}

#endif
