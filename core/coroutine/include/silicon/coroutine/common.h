#ifndef SILICON_COROUTINE_COMMON_H
#define SILICON_COROUTINE_COMMON_H

// 可移植属性宏：GCC/Clang 下展开为 __attribute__(x)（如 __ATTRIBUTE__(used) →
// __attribute__((used))，防止死代码消除）；MSVC 无等价物，展开为空（模板按需实例化，
// 不影响语义）。本仓库此前从未定义该宏，导致 MSVC 将 used 当裸标识符报 C2065/C2061。
#if defined(_MSC_VER)
#    define __ATTRIBUTE__(x)
#else
#    define __ATTRIBUTE__(x) __attribute__(x)
#endif

// 单 DLL 伞宏：silicon.dll 构建时 SILICON_EXPORT 由编译进本 DLL 的目标统一定义
// （dllexport）；消费方（siliconbuddy / 测试）不定义该宏 → dllimport。
// 各子模块保留自有的 *API 宏命名（此处 COROUTINE_API），但统一以 SILICON_EXPORT 为唯一闸门。
// 旧 coroutine/common.h 误用 SILICON_SHARED_LIB/SILICON_EXPORT 配对（与 xmake 定义的
// COROUTINE_SHARED_LIB/COROUTINE_EXPORT 不一致），导致 COROUTINE_API 恒为空、实际靠
// export_all 导出；现统一门控 SILICON_EXPORT。
#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SILICON_EXPORT)
#        define COROUTINE_API __declspec(dllexport)
#    else
#        define COROUTINE_API
#    endif
#else
#    if defined(SILICON_EXPORT)
#        define COROUTINE_API __attribute__((visibility("default")))
#    else
#        define COROUTINE_API
#    endif
#endif

#endif // SILICON_COROUTINE_COMMON_H
