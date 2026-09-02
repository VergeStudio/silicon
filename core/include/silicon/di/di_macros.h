#pragma once

// SILICON_DI_CONSTRUCTOR(sig) 展开为 di_constructor_type 别名（容器据此识别
// 构造注入签名）+ 原构造函数声明。宏无法从 C++20 模块导出，与
// silicon/proxy/proxy_macros.h 一致经普通头文件提供给消费方。
//
// 用法：
//   struct service {
//       SILICON_DI_CONSTRUCTOR(service(idep_a &a, idep_b &b));
//   };
#define SILICON_DI_CONSTRUCTOR(...)                                                 \
    using di_constructor_type [[maybe_unused]] =                            \
        ::silicon::di::constructor<__VA_ARGS__>;                                     \
    __VA_ARGS__
