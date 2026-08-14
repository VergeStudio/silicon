#ifndef SILICON_CONFIG_COMMON_H
#define SILICON_CONFIG_COMMON_H

// Platform detection using standard predefined macros (no custom macros needed).
// Windows: _WIN32, _WIN64
// Unix-like: __linux__, __APPLE__, etc.

#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(CONFIG_SHARED_LIB)
#        if defined(CONFIG_EXPORT)
#            define CONFIG_API __declspec(dllexport)
#        else
#            define CONFIG_API __declspec(dllimport)
#        endif
#    else
#        define CONFIG_API
#    endif
#else
#    if defined(CONFIG_SHARED_LIB)
#        if defined(CONFIG_EXPORT)
#            define CONFIG_API __attribute__((visibility("default")))
#        else
#            define CONFIG_API
#        endif
#    else
#        define CONFIG_API
#    endif
#endif

#endif // SILICON_CONFIG_COMMON_H
