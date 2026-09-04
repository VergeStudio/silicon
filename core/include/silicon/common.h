#ifndef SILICON_COMMON_H
#define SILICON_COMMON_H









#if defined(_MSC_VER)
#    define __ATTRIBUTE__(x)
#else
#    define __ATTRIBUTE__(x) __attribute__((x))
#endif




#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(CORE_EXPORT)
#        define CORE_API __declspec(dllexport)
#    else
#        define CORE_API
#    endif
#else
#    if defined(CORE_EXPORT)
#        define CORE_API __attribute__((visibility("default")))
#    else
#        define CORE_API
#    endif
#endif






#endif
