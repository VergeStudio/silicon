#ifndef SILICON_CORE_COMMON_H
#define SILICON_CORE_COMMON_H

// 平台判定统一使用 SILICON_PLATFORM_*（由根 xmake.lua 在构建顶层统一定义，
// 见该文件注释）。本头不再直接依赖编译器预定义宏。
// Windows: SILICON_PLATFORM_WINDOWS  |  Unix 族: SILICON_PLATFORM_UNIX
// 细分: APPLE / LINUX / BSD

#if defined(SILICON_PLATFORM_WINDOWS)
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
