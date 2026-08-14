module;

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

export module silicon.fs;
export import silicon.fs.error;

// proxy 的 dispatch 宏走头文件通道，本模块定义门面须在全局模块片段显式
// 包含，随后再 import silicon.proxy（宏不随 C++20 模块导出）。
#include <silicon/proxy/proxy_macros.h>

import silicon.proxy;

export namespace silicon::fs {

/// 统一错误返回类型：std::expected<T, std::error_code> 的别名。
/// 错误码来源：fs_error 枚举（make_error_code）或
/// 系统 errno 经 std::error_code{ec, std::generic_category()} 表达。
template<typename T>
using result = std::expected<T, std::error_code>;

// ── 类型擦除门面（silicon.proxy，替代原 i_file_system 抽象基类） ─────
//
// 目标类型无需继承任何基类，只要拥有下列同名成员即自动满足门面（鸭子类型）。
// 既有的具体类 win32_file_system / posix_file_system 直接接入，不再耦合继承体系。

PRO_DEF_MEM_DISPATCH(MemFsRead, read);
PRO_DEF_MEM_DISPATCH(MemFsWrite, write);
PRO_DEF_MEM_DISPATCH(MemFsReadBinary, read_binary);
PRO_DEF_MEM_DISPATCH(MemFsWriteBinary, write_binary);
PRO_DEF_MEM_DISPATCH(MemFsExists, exists);
PRO_DEF_MEM_DISPATCH(MemFsListDir, list_dir);
PRO_DEF_MEM_DISPATCH(MemFsCreateDirs, create_directories);

/// 文件系统门面：满足 read / write / read_binary / write_binary / exists /
/// list_dir / create_directories 七个 const 成员。
struct file_system_facade
    : silicon::proxy::facade_builder                                                           //
      ::add_convention<MemFsRead, result<std::string>(const std::string &) const>              //
      ::add_convention<MemFsWrite, result<void>(const std::string &, const std::string &) const> //
      ::add_convention<MemFsReadBinary, result<std::vector<std::byte>>(const std::string &) const> //
      ::add_convention<MemFsWriteBinary, result<void>(const std::string &, const std::vector<std::byte> &) const> //
      ::add_convention<MemFsExists, bool(const std::string &) const>                           //
      ::add_convention<MemFsListDir, result<std::vector<std::string>>(const std::string &) const> //
      ::add_convention<MemFsCreateDirs, bool(const std::string &) const>                        //
      ::build {};

/// 拥有所有权的类型擦除句柄（值语义）。
using file_system_proxy = silicon::proxy::proxy<file_system_facade>;

/// 非拥有观察视图，等价于裸指针但不要求继承。
using file_system_view = silicon::proxy::proxy_view<file_system_facade>;

/// 就地构造任意满足门面的目标类型并擦除为 file_system_proxy；句柄按值持有。
template<class T, class... Args>
[[nodiscard]] file_system_proxy make_file_system(Args &&...args) {
    return silicon::proxy::make_proxy<file_system_facade, T>(std::forward<Args>(args)...);
}

/// 工厂：返回当前平台的文件系统实现（具体类型由 SILICON_PLATFORM_* 宏选中），
/// 以类型擦除句柄承载，调用方无需感知具体平台类型。
file_system_proxy create_file_system();

} // namespace silicon::fs
