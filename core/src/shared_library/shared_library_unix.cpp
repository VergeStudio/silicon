module;
#include <memory>

#if defined(SILICON_PLATFORM_UNIX)
#    include <dlfcn.h>
#endif

#include <expected>
#include <mutex>
#include <string>
#include <system_error>

module silicon.library;

import silicon.platform;

#include "shared_library_impl.hpp"

#if defined(SILICON_PLATFORM_UNIX)

// 平台无关成员（构造函数/is_loaded/get_path）定义在公共实现单元 shared_library.cpp。
// 本文件仅提供 POSIX 差异成员：load/unload（dlopen 系）、prefix/suffix、find_symbol（dlsym）。
// 守卫与 shared_library_windows.cpp 的 SILICON_PLATFORM_WINDOWS 守卫互斥，恰好一个文件定义同组符号。
namespace silicon::library {

std::expected<void, std::error_code> shared_library::load(const std::string &path, int32_t) {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    if (impl_->handle_ != nullptr) {
        return std::unexpected(make_error_code(library_error::kAlreadyLoaded));
    }

    impl_->handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (impl_->handle_ == nullptr) {
        const char *err = dlerror();
        (void)err;
        return std::unexpected(make_error_code(library_error::kLoadFailed));
    }
    impl_->path_ = path;
    return {};
}

std::expected<void, std::error_code> shared_library::unload() {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    if (impl_->handle_ != nullptr) {
        dlclose(impl_->handle_);
        impl_->handle_ = nullptr;
    }
    return {};
}

std::string shared_library::prefix() {
    if constexpr (platform::os == platform::os_id::kCygwin) {
        return "cyg";
    } else {
        return "lib";
    }
}

std::string shared_library::suffix() {
    if constexpr (platform::os == platform::os_id::kMacOsX) {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
        return "d.dylib";
#else
        return ".dylib";
#endif
    } else if constexpr (platform::os == platform::os_id::kHpux) {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
        return "d.sl";
#else
        return ".sl";
#endif
    } else if constexpr (platform::os == platform::os_id::kCygwin) {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
        return "d.dll";
#else
        return ".dll";
#endif
    } else {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
        return "d.so";
#else
        return ".so";
#endif
    }
}

void *shared_library::find_symbol(const std::string &name) {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    void *result = nullptr;
    if (impl_->handle_) {
        result = dlsym(impl_->handle_, name.c_str());
    }
    return result;
}

} // namespace silicon::library

#endif // defined(SILICON_PLATFORM_UNIX)
