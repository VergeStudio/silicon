#ifndef SILICON_CLI_COMMON_H
#define SILICON_CLI_COMMON_H

// Platform detection via SILICON_PLATFORM_* macros (single source: xmake.lua root;
// see core/include/silicon/core/common.h). Do NOT use raw predefined OS macros.
// Windows: SILICON_PLATFORM_WINDOWS
// Unix-like: SILICON_PLATFORM_UNIX / LINUX / APPLE / BSD

#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(CLI_SHARED_LIB)
#        if defined(CLI_EXPORT)
#            define CLI_API __declspec(dllexport)
#        else
#            define CLI_API __declspec(dllimport)
#        endif
#    else
#        define CLI_API
#    endif
#else
#    if defined(CLI_SHARED_LIB)
#        if defined(CLI_EXPORT)
#            define CLI_API __attribute__((visibility("default")))
#        else
#            define CLI_API
#        endif
#    else
#        define CLI_API
#    endif
#endif

#endif // SILICON_CLI_COMMON_H
