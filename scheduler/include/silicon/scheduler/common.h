#ifndef SILICON_SCHEDULER_COMMON_H
#define SILICON_SCHEDULER_COMMON_H

#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SCHEDULER_SHARED_LIB)
#        if defined(SCHEDULER_EXPORT)
#            define SCHEDULER_API __declspec(dllexport)
#        else
#            define SCHEDULER_API __declspec(dllimport)
#        endif
#    else
#        define SCHEDULER_API
#    endif
#else
#    if defined(SCHEDULER_SHARED_LIB)
#        if defined(SCHEDULER_EXPORT)
#            define SCHEDULER_API __attribute__((visibility("default")))
#        else
#            define SCHEDULER_API
#        endif
#    else
#        define SCHEDULER_API
#    endif
#endif

// GCC attribute macro。GCC/Clang 扩展；MSVC 上展开为空。
// 原代码自 silicon.coroutine 下沉至本模块（sync_wait 等分区）时一并带用了该宏，
// 但本模块按依赖方向不得再依赖 coroutine（coroutine -> scheduler -> task），
// 故在此以 #ifndef 守卫提供本地定义，避免重复定义（若某 TU 同时引入 coroutine/common.h）。
#ifndef __ATTRIBUTE__
#    if defined(__GNUC__) && !defined(__clang__)
#        define __ATTRIBUTE__(attr) __attribute__((attr))
#    else
#        define __ATTRIBUTE__(attr)
#    endif
#endif

#endif // SILICON_SCHEDULER_COMMON_H
