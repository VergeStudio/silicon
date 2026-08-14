#ifndef SILICON_COROUTINE_COMMON_H
#define SILICON_COROUTINE_COMMON_H

// Platform detection using standard predefined macros.
#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SILICON_SHARED_LIB)
#        if defined(SILICON_EXPORT)
#            define COROUTINE_API __declspec(dllexport)
#        else
#            define COROUTINE_API __declspec(dllimport)
#        endif
#    else
#        define COROUTINE_API
#    endif
#else
#    if defined(SILICON_SHARED_LIB)
#        if defined(SILICON_EXPORT)
#            define COROUTINE_API __attribute__((visibility("default")))
#        else
#            define COROUTINE_API
#        endif
#    else
#        define COROUTINE_API
#    endif
#endif

// GCC attribute macro. This is a GCC extension; define it only for GCC and
// compilers that emulate GCC. Clang/MSVC leave it empty.
#if defined(__GNUC__) && !defined(__clang__)
#    define __ATTRIBUTE__(attr) __attribute__((attr))
#else
#    define __ATTRIBUTE__(attr)
#endif

#endif // SILICON_COROUTINE_COMMON_H
