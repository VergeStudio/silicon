#pragma once

// shared_library 的 PIMPL 实现体定义。
//
// 各平台实现单元（windows/unix/vx/hpux）都需要 impl 的完整类型，故集中于此私有
// 头文件共享；它不随 add_headerfiles 对外安装，也不属于模块接口的一部分。
//
// 使用约定：
//   1. 必须在 `module silicon.library;` 之后 include，因为 shared_library 由该模块
//      接口单元导出，全局模块片段中不可见。
//   2. 本头文件刻意不 include <mutex>/<string>：在模块 purview 内使用尖括号
//      include 会触发 -Winclude-angled-in-module-purview。调用方需在自身的全局
//      模块片段（`module;` 段）中先行包含这两个头。

namespace silicon::library {

struct shared_library::impl {
    std::string path_;
    void *handle_{nullptr};
    std::mutex mutex_;
};

} // namespace silicon::library
