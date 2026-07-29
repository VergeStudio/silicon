#ifndef SILICON_THREAD_COMMON_H
#define SILICON_THREAD_COMMON_H

#if defined(_WIN32) || defined(_WIN64)
#    if defined(THREAD_SHARED_LIB)
#        if defined(THREAD_EXPORT)
#            define THREAD_API __declspec(dllexport)
#        else
#            define THREAD_API __declspec(dllimport)
#        endif
#    else
#        define THREAD_API
#    endif
#else
#    if defined(THREAD_SHARED_LIB)
#        if defined(THREAD_EXPORT)
#            define THREAD_API __attribute__((visibility("default")))
#        else
#            define THREAD_API
#        endif
#    else
#        define THREAD_API
#    endif
#endif

#endif // SILICON_THREAD_COMMON_H
