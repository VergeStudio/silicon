#ifndef SILICON_COMMON_H
#define SILICON_COMMON_H

#if defined(_MSC_VER)
#    define __ATTRIBUTE__(x)
#else
#    define __ATTRIBUTE__(x) __attribute__((x))
#endif

#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SILICON_CORE_EXPORT)
#        define SILICON_CORE_API __declspec(dllexport)
#    else
#        define SILICON_CORE_API
#    endif
#else
#    if defined(SILICON_CORE_EXPORT)
#        define SILICON_CORE_API __attribute__((visibility("default")))
#    else
#        define SILICON_CORE_API
#    endif
#endif

#endif
