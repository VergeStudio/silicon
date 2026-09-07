module;

#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <silicon/common.h>

module silicon.fs;
import silicon.fs.error;
import silicon.proxy;

namespace silicon::fs {

template<class Derived>
class file_system_base {
  public:
    result<std::string> read(const std::string &path) const {
        std::ifstream f(to_path(path), std::ios::in | std::ios::binary);
        if(!f) return std::unexpected(make_error_code(fs_error::kOpenFailed));
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    result<std::vector<std::byte>> read_binary(const std::string &path) const {
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

    result<void> write_binary(const std::string &path, const std::vector<std::byte> &data) const {
        std::ofstream f(to_path(path), std::ios::out | std::ios::binary);
        if(!f) return std::unexpected(make_error_code(fs_error::kWriteFailed));
        if(!data.empty())
            f.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
        return {};
    }

    result<void> write(const std::string &path, const std::string &content) const {
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

    result<std::vector<std::string>> list_dir(const std::string &path) const {
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

#if defined(SILICON_PLATFORM_WINDOWS)

class win32_file_system: public file_system_base<win32_file_system> {
  public:
    std::string normalize_text(const std::string &content) const {
        std::string out;
        out.reserve(content.size() + content.size() / 8 + 1);
        for(char c: content) {
            if(c == '\n') out.push_back('\r');
            out.push_back(c);
        }
        return out;
    }

};

#elif defined(SILICON_PLATFORM_UNIX)

class posix_file_system: public file_system_base<posix_file_system> {
  public:
    bool create_directories(const std::string &path) const {
        std::error_code ec;
        auto p = to_path(path);
        bool made = std::filesystem::create_directories(p, ec);
        if(ec) return false;
        std::filesystem::permissions(p, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec, std::filesystem::perm_options::replace, ec);
        return made;
    }

};

#endif

SILICON_CORE_API file_system_proxy create_file_system() {
#if defined(SILICON_PLATFORM_WINDOWS)
    return make_file_system<win32_file_system>();
#else
    return make_file_system<posix_file_system>();
#endif
}

}
