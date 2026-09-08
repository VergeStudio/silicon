module;

#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <string>
#include <string_view>
#include <vector>

#include <tuple>

#include <silicon/proxy/proxy_macros.h>

#include <silicon/common.h>
export module silicon.fs;
export import silicon.fs.error;

import silicon.proxy;
import silicon.error;

export namespace silicon::fs {

template<typename T>

PRO_DEF_MEM_DISPATCH(MemFsRead, read);
PRO_DEF_MEM_DISPATCH(MemFsWrite, write);
PRO_DEF_MEM_DISPATCH(MemFsReadBinary, read_binary);
PRO_DEF_MEM_DISPATCH(MemFsWriteBinary, write_binary);
PRO_DEF_MEM_DISPATCH(MemFsExists, exists);
PRO_DEF_MEM_DISPATCH(MemFsListDir, list_dir);
PRO_DEF_MEM_DISPATCH(MemFsCreateDirs, create_directories);

struct file_system_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemFsRead, silicon::error::result<std::string>(const std::string &) const>
      ::add_convention<MemFsWrite, silicon::error::result<void>(const std::string &, const std::string &) const>
      ::add_convention<MemFsReadBinary, silicon::error::result<std::vector<std::byte>>(const std::string &) const>
      ::add_convention<MemFsWriteBinary, silicon::error::result<void>(const std::string &, const std::vector<std::byte> &) const>
      ::add_convention<MemFsExists, bool(const std::string &) const>
      ::add_convention<MemFsListDir, silicon::error::result<std::vector<std::string>>(const std::string &) const>
      ::add_convention<MemFsCreateDirs, bool(const std::string &) const>
      ::build {};

using file_system_proxy = silicon::proxy::proxy<file_system_facade>;

using file_system_view = silicon::proxy::proxy_view<file_system_facade>;

template<class T, class... Args>
[[nodiscard]] file_system_proxy make_file_system(Args &&...args) {
    return silicon::proxy::make_proxy<file_system_facade, T>(std::forward<Args>(args)...);
}

SILICON_CORE_API file_system_proxy create_file_system();

}

// 平台无关共享实现（模板），供各平台文件继承；非 export，仅模块内可见。
namespace silicon::fs {

template<class Derived>
class file_system_base {
  public:
    silicon::error::result<std::string> read(const std::string &path) const {
        std::ifstream f(to_path(path), std::ios::in | std::ios::binary);
        if(!f) return std::unexpected(make_error_code(fs_error::kOpenFailed));
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    silicon::error::result<std::vector<std::byte>> read_binary(const std::string &path) const {
        std::ifstream f(to_path(path), std::ios::in | std::ios::binary);
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

    silicon::error::result<void> write_binary(const std::string &path, const std::vector<std::byte> &data) const {
        std::ofstream f(to_path(path), std::ios::out | std::ios::binary);
        if(!f) return std::unexpected(make_error_code(fs_error::kWriteFailed));
        if(!data.empty())
            f.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        return {};
    }

    silicon::error::result<void> write(const std::string &path, const std::string &content) const {
        const std::string normalized = static_cast<const Derived &>(*this).normalize_text(content);
        std::vector<std::byte> bytes(normalized.size());
        for(std::size_t i = 0; i < normalized.size(); ++i)
            bytes[i] = static_cast<std::byte>(normalized[i]);
        return write_binary(path, bytes);
    }

    bool exists(const std::string &path) const {
        std::error_code ec;
        return std::filesystem::exists(to_path(path), ec);
    }

    silicon::error::result<std::vector<std::string>> list_dir(const std::string &path) const {
        std::error_code ec;
        auto it = std::filesystem::directory_iterator(to_path(path), ec);
        if(ec) return std::unexpected(make_error_code(fs_error::kOpenFailed));
        std::vector<std::string> entries;
        for(const auto &entry: it)
            entries.push_back(entry.path().filename().string());
        return entries;
    }

    bool create_directories(const std::string &path) const {
        std::error_code ec;
        return std::filesystem::create_directories(to_path(path), ec);
    }

    std::string normalize_text(const std::string &content) const { return content; }

  protected:
    static std::filesystem::path to_path(const std::string &p) { return std::filesystem::path{p}; }
};

}
