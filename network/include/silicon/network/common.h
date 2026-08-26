#ifndef SILICON_NETWORK_COMMON_H
#define SILICON_NETWORK_COMMON_H

// 单 DLL 伞宏：silicon.dll 构建时 SILICON_EXPORT 由编译进本 DLL 的目标统一定义
// （dllexport）；消费方（siliconbuddy / 测试）不定义该宏 → dllimport。
// 各子模块保留自有的 *API 宏命名（此处 NET_API），但统一以 SILICON_EXPORT 为唯一闸门。
#if defined(SILICON_PLATFORM_WINDOWS)
#    if defined(SILICON_EXPORT)
#        define NET_API __declspec(dllexport)
#    else
#        define NET_API __declspec(dllimport)
#    endif
#else
#    if defined(SILICON_EXPORT)
#        define NET_API __attribute__((visibility("default")))
#    else
#        define NET_API
#    endif
#endif

#endif // SILICON_NETWORK_COMMON_H
