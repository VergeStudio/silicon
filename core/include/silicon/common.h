#ifndef SILICON_COMMON_H
#define SILICON_COMMON_H

// silicon 全仓统一公共头。

// 平台检测：SILICON_PLATFORM_* 宏由根 xmake.lua 按目标平台统一定义
// （Windows: SILICON_PLATFORM_WINDOWS；Unix-like: SILICON_PLATFORM_UNIX /
// LINUX / APPLE / BSD）。禁止直接使用编译器预定义 OS 宏。

// 可移植属性宏：GCC/Clang 下展开为 __attribute__(x)（如 __ATTRIBUTE__(used)，
// 防止死代码消除）；MSVC 无等价物，展开为空（模板按需实例化，不影响语义）。
#if defined(_MSC_VER)
#    define __ATTRIBUTE__(x)
#else
#    define __ATTRIBUTE__(x) __attribute__((x))
#endif

// 单 DLL 伞宏：聚合库构建时 CORE_EXPORT 由编译进该 DLL 的 core 目标
// 统一定义（dllexport）；消费方（测试/应用）不定义该宏 → dllimport（展开为空，
// 符号经导入库解析）。导出实体统一以 CORE_API 标注。
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

// MSVC 注意：模块接口中声明、实现单元内定义的错误类别实例（如
// di_error_category_instance / fs_category_impl）与仅含内联虚函数的导出类
// （如 di_category_impl / fs_category_impl）的 vtable，MSVC 不会自动经模块
// 链接导出到聚合库导入库，须显式以对应 *API 宏标注，否则跨 DLL 消费方
// 出现 LNK2001。
#endif // SILICON_COMMON_H
