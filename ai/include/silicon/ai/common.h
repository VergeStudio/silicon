#ifndef SILICON_AI_COMMON_H
#define SILICON_AI_COMMON_H

// Platform detection using standard predefined macros (no custom macros needed).
// Windows: _WIN32, _WIN64
// Unix-like: __linux__, __APPLE__, etc.

#if defined(_WIN32) || defined(_WIN64)
#    if defined(AI_SHARED_LIB)
#        if defined(AI_EXPORT)
#            define AI_API __declspec(dllexport)
#        else
#            define AI_API __declspec(dllimport)
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
