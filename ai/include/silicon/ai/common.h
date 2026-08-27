#ifndef SILICON_AI_COMMON_H
#define SILICON_AI_COMMON_H

// Platform detection via SILICON_PLATFORM_* macros (single source: xmake.lua root;
// see core/include/silicon/core/common.h). Do NOT use raw predefined OS macros.
// Windows: SILICON_PLATFORM_WINDOWS
// Unix-like: SILICON_PLATFORM_UNIX / LINUX / APPLE / BSD

#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(AI_SHARED_LIB)
#        if defined(AI_EXPORT)
#            define AI_API __declspec(dllexport)
#        else
#            define AI_API
#        endif
#    else
#        define AI_API
#    endif
#else
#    if defined(AI_SHARED_LIB)
#        if defined(AI_EXPORT)
#            define AI_API __attribute__((visibility("default")))
#        else
#            define AI_API
#        endif
#    else
#        define AI_API
#    endif
#endif

#endif // SILICON_AI_COMMON_H
