#pragma once

// POSIX 平台实现（SILICON_PLATFORM_UNIX 由顶层 xmake.lua 在 linux/unix/bsd 下定义）。
// 本文件经 fs.cppm 按宏选中后 #include 进 silicon.fs 模块。

#include <filesystem>

#include "silicon/fs/fs_common.hpp"

export namespace silicon::fs {

/// POSIX 文件系统实现（Linux/Unix/macOS）。
/// 文本保持 LF；目录创建后显式设置 0755 权限。
class PosixFileSystem: public FileSystemBase {
  public:
    bool create_directories(const std::string &path) const override {
        std::error_code ec;
        auto p = to_path(path);
        bool made = std::filesystem::create_directories(p, ec);
        if(ec) return false;
        std::filesystem::permissions(p, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec, std::filesystem::perm_options::replace, ec);
        return made;
    }
    // normalize_text 复用基类默认实现（保持 LF）
};

} // namespace silicon::fs
