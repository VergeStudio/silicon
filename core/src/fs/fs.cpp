// 实现单元：silicon.fs
// 原 fs_common.hpp / fs_win.hpp / fs_posix.hpp 的平台实现已并入此模块实现单元，
// 按 SILICON_PLATFORM_* 宏在编译期选用对应平台实现（宏由顶层 xmake.lua 定义）。
// create_file_system() 此前仅有声明、无定义，现于此补齐，返回 file_system_proxy。
module;

#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

// FS_API 宏（dllexport/dlimport 闸门）：create_file_system 定义处须可见，否则 FS_API 展开为空。
#include <silicon/common.h>

module silicon.fs;
import silicon.fs.error;
import silicon.proxy;

namespace silicon::fs {

/// 平台无关的文件系统实现基类（CRTP，无虚函数、无抽象接口耦合）。
/// win32_file_system / posix_file_system 以具体派生类型实例化 Derived，
/// write() 经 CRTP 静态分派到派生类的 NormalizeText，避免虚表。
template<class Derived>
class file_system_base {
  public:
    result<std::string> read(const std::string &path) const {
        std::ifstream f(ToPath(path), std::ios::in | std::ios::binary);
        if(!f) return std::unexpected(make_error_code(fs_error::kOpenFailed));
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    result<std::vector<std::byte>> read_binary(const std::string &path) const {
        std::ifstream f(ToPath(path), std::ios::in | std::ios::binary);
        if(!f) return std::unexpected(make_error_code(fs_error::kOpenFailed));
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

    result<void> write_binary(const std::string &path, const std::vector<std::byte> &data) const {
        std::ofstream f(ToPath(path), std::ios::out | std::ios::binary);
        if(!f) return std::unexpected(make_error_code(fs_error::kWriteFailed));
        if(!data.empty())
            f.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        return {};
    }

    result<void> write(const std::string &path, const std::string &content) const {
        const std::string normalized = static_cast<const Derived &>(*this).NormalizeText(content);
        std::vector<std::byte> bytes(normalized.size());
        for(std::size_t i = 0; i < normalized.size(); ++i)
            bytes[i] = static_cast<std::byte>(normalized[i]);
        return write_binary(path, bytes);
    }

    bool exists(const std::string &path) const {
        std::error_code ec;
        return std::filesystem::exists(ToPath(path), ec);
    }

    result<std::vector<std::string>> list_dir(const std::string &path) const {
        std::error_code ec;
        auto it = std::filesystem::directory_iterator(ToPath(path), ec);
        if(ec) return std::unexpected(make_error_code(fs_error::kOpenFailed));
        std::vector<std::string> entries;
        for(const auto &entry: it)
            entries.push_back(entry.path().filename().string());
        return entries;
    }

    bool create_directories(const std::string &path) const {
        std::error_code ec;
        return std::filesystem::create_directories(ToPath(path), ec);
    }

    /// 平台钩子：文本写入前的换行符规范化（默认原样，POSIX 直接复用）。
    /// 派生类可隐藏以提供平台特定行为；write() 经 CRTP 调用派生版本。
    std::string NormalizeText(const std::string &content) const { return content; }

  protected:
    static std::filesystem::path ToPath(const std::string &p) { return std::filesystem::path{p}; }
};

#if defined(SILICON_PLATFORM_WINDOWS)

/// Windows 文件系统实现。
/// 文本写入按 Windows 约定归一化为 CRLF；目录创建使用系统默认权限。
class win32_file_system: public file_system_base<win32_file_system> {
  public:
    std::string NormalizeText(const std::string &content) const {
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
class posix_file_system: public file_system_base<posix_file_system> {
  public:
    bool create_directories(const std::string &path) const {
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

FS_API file_system_proxy create_file_system() {
#if defined(SILICON_PLATFORM_WINDOWS)
    return make_file_system<win32_file_system>();
#else
    return make_file_system<posix_file_system>();
#endif
}

} // namespace silicon::fs
