#ifndef SILICON_CORE_COMMON_H
#define SILICON_CORE_COMMON_H

// Platform detection using standard predefined macros (no custom macros needed).
// Windows: _WIN32, _WIN64
// Unix-like: __linux__, __APPLE__, etc.

#if defined(_WIN32) || defined(_WIN64)
#    if defined(CORE_SHARED_LIB)
#        if defined(CORE_EXPORT)
#            define CORE_API __declspec(dllexport)
#        else
#            define CORE_API __declspec(dllimport)
#        endif
#    else
#        define CORE_API
#    endif
#else
#    if defined(CORE_SHARED_LIB)
#        if defined(CORE_EXPORT)
#            define CORE_API __attribute__((visibility("default")))
#        else
#            define CORE_API
#        endif
#    else
#        define CORE_API
#    endif
#endif

#endif // SILICON_CORE_COMMON_H
