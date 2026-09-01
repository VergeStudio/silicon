#ifndef SILICON_SCHEDULER_COMMON_H
#define SILICON_SCHEDULER_COMMON_H

// 可移植属性宏：GCC/Clang 下展开为 __attribute__(x)（如 __ATTRIBUTE__(used) →
// __attribute__((used))，防止死代码消除）；MSVC 无等价物，展开为空（模板按需实例化，
// 不影响语义）。本仓库此前从未定义该宏，导致 MSVC 将 used 当裸标识符报 C2065/C2061。
#if defined(_MSC_VER)
#    define __ATTRIBUTE__(x)
#else
#    define __ATTRIBUTE__(x) __attribute__((x))
#endif

// 单 DLL 伞宏：silicon.dll 构建时 SILICON_EXPORT 由编译进本 DLL 的目标统一定义
// （dllexport）；消费方（siliconbuddy / 测试）不定义该宏 → dllimport。
// 各子模块保留自有的 *API 宏命名（此处 SCHEDULER_API），但统一以 SILICON_EXPORT 为唯一闸门。
#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SILICON_EXPORT)
#        define SCHEDULER_API __declspec(dllexport)
#    else
#        define SCHEDULER_API
#    endif
#else
#    if defined(SILICON_EXPORT)
#        define SCHEDULER_API __attribute__((visibility("default")))
#    else
#        define SCHEDULER_API
#    endif
#endif

#endif // SILICON_SCHEDULER_COMMON_H
