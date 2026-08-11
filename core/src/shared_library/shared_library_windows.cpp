module;
#include <memory>

#if defined(_WIN32) || defined(_WIN64)
#    include <Windows.h>
#endif

#include <expected>
#include <mutex>
#include <string>
#include <system_error>

module silicon.library;

import silicon.platform;
import silicon.error;

#include "shared_library_impl.hpp"

#if defined(_WIN32) || defined(_WIN64)

#include "libloaderapi.h"
#include "minwindef.h"

// 平台无关成员（构造函数/is_loaded/get_path）定义在公共实现单元 shared_library.cpp。
// 本文件仅提供 Windows 差异成员：load/unload（LoadLibrary 系）、prefix/suffix、
// find_symbol（GetProcAddress）。守卫与 shared_library_unix.cpp 的 unix 系守卫互斥。
namespace silicon::library {

auto shared_library::load(const std::string &path, int32_t flags) -> std::expected<void, std::error_code> {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    impl_->handle_ = LoadLibrary(path.c_str());
    if (impl_->handle_ == nullptr) {
        return std::unexpected(silicon::error::make_error_code(silicon::error::library_error::kLoadFailed));
    }
    impl_->path_ = path;
    return {};
}

auto shared_library::unload() -> std::expected<void, std::error_code> {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    if (impl_->handle_ != nullptr) {
        FreeLibrary(static_cast<HMODULE>(impl_->handle_));
        impl_->handle_ = nullptr;
    }
    impl_->path_.clear();
    return {};
}

std::string shared_library::prefix() {
    return "";
}

std::string shared_library::suffix() {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
    return "d.dll";
#else
    return ".dll";
#endif
}

void *shared_library::find_symbol(const std::string &name) {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    if (impl_->handle_ != nullptr) {
#if defined(_WIN32_WCE)
        std::wstring uname;
        UnicodeConverter::toUTF16(name, uname);
        return static_cast<void *>(GetProcAddressW(static_cast<HMODULE>(impl_->handle_), uname.c_str()));
#else
        // 函数指针 → 对象指针的 static_cast 是 MS 扩展（-Wmicrosoft-cast），
        // 标准写法需经 reinterpret_cast。
        return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(impl_->handle_), name.data()));
#endif
    }

    return nullptr;
}

} // namespace silicon::library

#endif // defined(_WIN32) || defined(_WIN64)
