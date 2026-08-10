// 实现单元：silicon.fs
// 原 fs_common.hpp / fs_win.hpp / fs_posix.hpp 的平台实现已并入此模块实现单元，
// 按 SILICON_PLATFORM_* 宏在编译期选用对应平台实现（宏由顶层 xmake.lua 定义）。
// create_file_system() 此前仅有声明、无定义，现于此补齐。
module;

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

module silicon.fs;

import silicon.exception;

namespace silicon::fs {

/// 平台无关的文件系统实现基类，被 win32_file_system / posix_file_system 继承。
class file_system_base: public i_file_system {
  public:
    result<std::string> read(const std::string &path) const override {
        std::ifstream f(ToPath(path), std::ios::in | std::ios::binary);
        if(!f) return silicon::exception::fs_error{"cannot open: " + path};
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    result<std::vector<std::byte>> read_binary(const std::string &path) const override {
        std::ifstream f(ToPath(path), std::ios::in | std::ios::binary);
        if(!f) return silicon::exception::fs_error{"cannot open: " + path};
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

    result<void> write_binary(const std::string &path, const std::vector<std::byte> &data) const override {
        std::ofstream f(ToPath(path), std::ios::out | std::ios::binary);
        if(!f) return silicon::exception::fs_error{"cannot write: " + path};
        if(!data.empty())
            f.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        return {};
    }

    result<void> write(const std::string &path, const std::string &content) const override {
        const std::string normalized = NormalizeText(content);
        std::vector<std::byte> bytes(normalized.size());
        for(std::size_t i = 0; i < normalized.size(); ++i)
            bytes[i] = static_cast<std::byte>(normalized[i]);
        return write_binary(path, bytes);
    }

    bool exists(const std::string &path) const override {
        std::error_code ec;
        return std::filesystem::exists(ToPath(path), ec);
    }

    result<std::vector<std::string>> list_dir(const std::string &path) const override {
        std::error_code ec;
        auto it = std::filesystem::directory_iterator(ToPath(path), ec);
        if(ec) return silicon::exception::fs_error{"cannot list: " + path};
        std::vector<std::string> entries;
        for(const auto &entry: it)
            entries.push_back(entry.path().filename().string());
        return entries;
    }

    bool create_directories(const std::string &path) const override {
        std::error_code ec;
        return std::filesystem::create_directories(ToPath(path), ec);
    }

    /// 平台钩子：文本写入前的换行符规范化（默认原样，POSIX 直接复用）
    virtual std::string NormalizeText(const std::string &content) const { return content; }

  protected:
    static std::filesystem::path ToPath(const std::string &p) { return std::filesystem::path{p}; }
};

#if defined(SILICON_PLATFORM_WINDOWS)

/// Windows 文件系统实现。
/// 文本写入按 Windows 约定归一化为 CRLF；目录创建使用系统默认权限。
class win32_file_system: public file_system_base {
  public:
    std::string NormalizeText(const std::string &content) const override {
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

#elif defined(SILICON_PLATFORM_UNIX)

/// POSIX 文件系统实现（Linux/Unix/macOS）。
/// 文本保持 LF；目录创建后显式设置 0755 权限。
class posix_file_system: public file_system_base {
  public:
    bool create_directories(const std::string &path) const override {
        std::error_code ec;
        auto p = ToPath(path);
        bool made = std::filesystem::create_directories(p, ec);
        if(ec) return false;
        std::filesystem::permissions(p, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec, std::filesystem::perm_options::replace, ec);
        return made;
    }
    // NormalizeText 复用基类默认实现（保持 LF）
};

#endif

std::unique_ptr<i_file_system> create_file_system() {
#if defined(SILICON_PLATFORM_WINDOWS)
    return std::make_unique<win32_file_system>();
#else
    return std::make_unique<posix_file_system>();
#endif
}

} // namespace silicon::fs
