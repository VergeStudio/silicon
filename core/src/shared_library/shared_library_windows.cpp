module;
#include <memory>

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <Windows.h>
#endif

#include <expected>
#include <mutex>
#include <string>
#include <system_error>

module silicon.library;

import silicon.platform;

#include "shared_library_impl.h"

#if defined(SILICON_PLATFORM_WINDOWS)

#include "libloaderapi.h"
#include "minwindef.h"




namespace silicon::library {

std::expected<void, std::error_code> shared_library::load(const std::string &path, int32_t) {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    impl_->handle_ = LoadLibrary(path.c_str());
    if (impl_->handle_ == nullptr) {
        return std::unexpected(make_error_code(library_error::kLoadFailed));
    }
    impl_->path_ = path;
    return {};
}

std::expected<void, std::error_code> shared_library::unload() {
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


        return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(impl_->handle_), name.data()));
#endif
    }

    return nullptr;
}

}

#endif
