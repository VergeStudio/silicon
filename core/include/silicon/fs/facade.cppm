module;

#include <cstddef>
#include <expected>
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
