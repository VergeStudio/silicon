#ifndef SILICON_FS_COMMON_H
#define SILICON_FS_COMMON_H

// 单 DLL 伞宏：core.dll 构建时 SILICON_EXPORT 由编译进本 DLL 的 core 目标统一定义
// （dllexport）；消费方（测试/应用）不定义该宏 → dllimport。
// 各子模块保留自有的 *API 宏命名（此处 FS_API），但统一以 SILICON_EXPORT 为唯一闸门。
// 注意：模块接口中声明、实现单元（fs.cpp）中定义的自由函数（create_file_system）
// 与仅含内联虚函数的导出类（fs_category_impl）的 vtable，MSVC 不会自动经模块链接
// 导出到 core.dll 导入库，须显式以 FS_API 标注，否则跨 DLL 消费方出现 LNK2001。
#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SILICON_EXPORT)
#        define FS_API __declspec(dllexport)
#    else
#        define FS_API __declspec(dllimport)
#    endif
#else
#    if defined(SILICON_EXPORT)
#        define FS_API __attribute__((visibility("default")))
#    else
#        define FS_API
#    endif
#endif

#endif // SILICON_FS_COMMON_H
