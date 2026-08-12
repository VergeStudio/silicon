module;

#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

export module silicon.fs;

export namespace silicon::fs {

/// 统一错误返回类型：std::expected<T, std::error_code> 的别名。
/// 错误码来源：fs_error 枚举（make_error_code）或
/// 系统 errno 经 std::error_code{ec, std::generic_category()} 表达。
template<typename T>
using result = std::expected<T, std::error_code>;

/// 文件系统语义错误枚举（专属 category：silicon.fs）。
enum class fs_error {
    kOpenFailed = 1,
    kReadFailed,
    kWriteFailed,
    kNotExist,
    kPermissionDenied,
    kNotDirectory,
    kAlreadyExists,
    kUnknown,
};

/// 返回 fs_error 专属 error_category（name() == "silicon.fs"）。
[[nodiscard]] const std::error_category &fs_category() noexcept;

/// fs_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] std::error_code make_error_code(fs_error e) noexcept;

/// 文件系统抽象（统一接口）。
/// 文本 read/write 平台无关地以 UTF-8 表达；平台相关细节
/// （Windows 文本 CRLF 归一化、POSIX 目录默认权限）由具体实现处理。
class i_file_system {
  public:
    virtual ~i_file_system() = default;

    /// 文本读取，返回 UTF-8 内容
    virtual result<std::string> read(const std::string &path) const = 0;
    /// 文本写入（平台相关：Windows 归一化为 CRLF，POSIX 保持 LF）
    virtual result<void> write(const std::string &path, const std::string &content) const = 0;

    /// 二进制读取
    virtual result<std::vector<std::byte>> read_binary(const std::string &path) const = 0;
    /// 二进制写入
    virtual result<void> write_binary(const std::string &path, const std::vector<std::byte> &data) const = 0;

    virtual bool exists(const std::string &path) const = 0;
    virtual result<std::vector<std::string>> list_dir(const std::string &path) const = 0;
    virtual bool create_directories(const std::string &path) const = 0;
};

} // namespace silicon::fs

// ── 分平台实现（file_system_base / win32_file_system / posix_file_system） ──
// 已并入模块实现单元 fs/src/fs.cpp：按 SILICON_PLATFORM_* 宏在编译期选用
// 对应平台实现（宏由顶层 xmake.lua 定义），create_file_system() 亦在该单元定义。

export namespace silicon::fs {

/// 工厂：返回当前平台的文件系统实现（具体类型由上面选中的头文件提供）。
std::unique_ptr<i_file_system> create_file_system();

} // namespace silicon::fs
