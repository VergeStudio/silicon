#ifndef SILICON_COROUTINE_COMMON_H
#define SILICON_COROUTINE_COMMON_H

// Platform detection using standard predefined macros.
#if defined(_WIN32) || defined(_WIN64)
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

#endif // SILICON_COROUTINE_COMMON_H
