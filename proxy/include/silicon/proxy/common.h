#ifndef SILICON_PROXY_COMMON_H
#define SILICON_PROXY_COMMON_H

#if defined(_WIN32) || defined(_WIN64)
#    if defined(PROXY_SHARED_LIB)
#        if defined(PROXY_EXPORT)
#            define PROXY_API __declspec(dllexport)
#        else
#            define PROXY_API __declspec(dllimport)
#        endif
#    else
#        define PROXY_API
#    endif
#else
#    if defined(PROXY_SHARED_LIB)
#        if defined(PROXY_EXPORT)
#            define PROXY_API __attribute__((visibility("default")))
#        else
#            define PROXY_API
#        endif
#    else
#        define PROXY_API
#    endif
#endif

#endif // SILICON_PROXY_COMMON_H
