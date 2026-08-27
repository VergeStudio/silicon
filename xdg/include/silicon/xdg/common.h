#ifndef SILICON_XDG_COMMON_H
#define SILICON_XDG_COMMON_H

// 单 DLL 伞宏：core.dll 构建时 SILICON_EXPORT 由编译进本 DLL 的 core 目标统一定义
// （dllexport）；消费方（测试/应用）不定义该宏 → dllimport。
// 各子模块保留自有的 *API 宏命名（此处 XDG_API），但统一以 SILICON_EXPORT 为唯一闸门。
// 注意：模块接口中声明、实现单元（xdg.cpp）中定义的自由函数（home_dir / *home /
// *dirs 等）MSVC 不会自动经模块链接导出到 core.dll 导入库，须显式以 XDG_API 标注，
// 否则跨 DLL 消费方出现 LNK2001。
#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SILICON_EXPORT)
#        define XDG_API __declspec(dllexport)
#    else
#        define XDG_API __declspec(dllimport)
#    endif
#else
#    if defined(SILICON_EXPORT)
#        define XDG_API __attribute__((visibility("default")))
#    else
#        define XDG_API
#    endif
#endif

#endif // SILICON_XDG_COMMON_H
