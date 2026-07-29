#ifndef SILICON_NET_COMMON_H
#define SILICON_NET_COMMON_H

#if defined(_WIN32) || defined(_WIN64)
#    if defined(NET_SHARED_LIB)
#        if defined(NET_EXPORT)
#            define NET_API __declspec(dllexport)
#        else
#            define NET_API __declspec(dllimport)
#        endif
#    else
#        define NET_API
#    endif
#else
#    if defined(NET_SHARED_LIB)
#        if defined(NET_EXPORT)
#            define NET_API __attribute__((visibility("default")))
#        else
#            define NET_API
#        endif
#    else
#        define NET_API
#    endif
#endif

#endif // SILICON_NET_COMMON_H
