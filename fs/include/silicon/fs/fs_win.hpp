#pragma once

// Windows 平台实现（SILICON_PLATFORM_WINDOWS 由顶层 xmake.lua 在 windows 下定义）。
// 本文件经 fs.cppm 按宏选中后 #include 进 silicon.fs 模块。

#include "silicon/fs/fs_common.hpp"

export namespace silicon::fs {

/// Windows 文件系统实现。
/// 文本写入按 Windows 约定归一化为 CRLF；目录创建使用系统默认权限。
class Win32FileSystem: public FileSystemBase {
  public:
    std::string normalize_text(const std::string &content) const override {
        std::string out;
        out.reserve(content.size() + content.size() / 8 + 1);
        for(char c: content) {
            if(c == '\n') out.push_back('\r');
            out.push_back(c);
        }
        return out;
    }
    // create_directories 复用基类默认实现（std::filesystem 在 Windows 上行为正确）
};

} // namespace silicon::fs
