#ifndef SILICON_CLI_COMMON_H
#define SILICON_CLI_COMMON_H

// Platform detection using standard predefined macros (no custom macros needed).
// Windows: _WIN32, _WIN64
// Unix-like: __linux__, __APPLE__, etc.

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
