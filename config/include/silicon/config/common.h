#ifndef SILICON_CONFIG_COMMON_H
#define SILICON_CONFIG_COMMON_H

// Platform detection via SILICON_PLATFORM_* macros (single source: xmake.lua root;
// see core/include/silicon/core/common.h). Do NOT use raw predefined OS macros.
// Windows: SILICON_PLATFORM_WINDOWS
// Unix-like: SILICON_PLATFORM_UNIX / LINUX / APPLE / BSD

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
