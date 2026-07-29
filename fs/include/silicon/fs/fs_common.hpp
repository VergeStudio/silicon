#pragma once

// 平台无关的文件系统实现基类，被 Win32FileSystem / PosixFileSystem 继承。
// 本文件经 fs.cppm 按 SILICON_PLATFORM_* 宏选中对应平台头文件后，
// 间接 #include 进 silicon.fs 模块翻译单元，故可直接使用模块内已导出的
// IFileSystem / Result / FsError 等类型。

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace silicon::fs {

/// 共享逻辑基于 std::filesystem；平台相关差异通过两个钩子下放：
///  - normalize_text()：文本写入前的换行符规范化（Windows 覆写为 CRLF）
///  - create_directories()：目录创建（POSIX 覆写以设置默认权限）
class FileSystemBase: public IFileSystem {
  public:
    Result<std::string> read(const std::string &path) const override {
        std::ifstream f(to_path(path), std::ios::in | std::ios::binary);
        if(!f) return silicon::exception::FsError{"cannot open: " + path};
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    Result<std::vector<std::byte>> read_binary(const std::string &path) const override {
        std::ifstream f(to_path(path), std::ios::in | std::ios::binary);
        if(!f) return silicon::exception::FsError{"cannot open: " + path};
        std::vector<std::byte> out;
        f.seekg(0, std::ios::end);
        const auto sz = static_cast<std::size_t>(f.tellg());
        f.seekg(0, std::ios::beg);
        if(sz > 0) {
            out.resize(sz);
            f.read(reinterpret_cast<char *>(out.data()), static_cast<std::streamsize>(sz));
        }
        return out;
    }

    Result<void> write_binary(const std::string &path, const std::vector<std::byte> &data) const override {
        std::ofstream f(to_path(path), std::ios::out | std::ios::binary);
        if(!f) return silicon::exception::FsError{"cannot write: " + path};
        if(!data.empty())
            f.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        return {};
    }

    Result<void> write(const std::string &path, const std::string &content) const override {
        const std::string normalized = normalize_text(content);
        std::vector<std::byte> bytes(normalized.size());
        for(std::size_t i = 0; i < normalized.size(); ++i)
            bytes[i] = static_cast<std::byte>(normalized[i]);
        return write_binary(path, bytes);
    }

    bool exists(const std::string &path) const override {
        std::error_code ec;
        return std::filesystem::exists(to_path(path), ec);
    }

    Result<std::vector<std::string>> list_dir(const std::string &path) const override {
        std::error_code ec;
        auto it = std::filesystem::directory_iterator(to_path(path), ec);
        if(ec) return silicon::exception::FsError{"cannot list: " + path};
        std::vector<std::string> entries;
        for(const auto &entry: it)
            entries.push_back(entry.path().filename().string());
        return entries;
    }

    bool create_directories(const std::string &path) const override {
        std::error_code ec;
        return std::filesystem::create_directories(to_path(path), ec);
    }

    /// 平台钩子：文本写入前的换行符规范化（默认原样，POSIX 直接复用）
    virtual std::string normalize_text(const std::string &content) const { return content; }

  protected:
    static std::filesystem::path to_path(const std::string &p) { return std::filesystem::path{p}; }
};

} // namespace silicon::fs
