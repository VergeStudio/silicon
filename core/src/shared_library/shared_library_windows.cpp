module;
#include <memory>

#if defined(_WIN32) || defined(_WIN64)
#    include <Windows.h>
#endif

#include <mutex>
#include <string>

module silicon.library;

import silicon.platform;
import silicon.exception;

#include "shared_library_impl.hpp"

#if defined(_WIN32) || defined(_WIN64)

#include "libloaderapi.h"
#include "minwindef.h"

namespace silicon::library {

SharedLibrary::SharedLibrary() : impl_(std::make_unique<Impl>()) {
}

void SharedLibrary::Load(const std::string &path, int32_t flags) {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    impl_->handle_ = LoadLibrary(path.c_str());
    if (impl_->handle_ == nullptr) {
        throw silicon::exception::RuntimeError("Could not load library: " + path);
    }
    impl_->path_ = path;
}

void SharedLibrary::Unload() {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    if (impl_->handle_ != nullptr) {
        FreeLibrary(static_cast<HMODULE>(impl_->handle_));
        impl_->handle_ = nullptr;
    }
    impl_->path_.clear();
}

bool SharedLibrary::IsLoaded() const {
    return impl_->handle_ != nullptr;
}

const std::string &SharedLibrary::GetPath() const {
    return impl_->path_;
}

std::string SharedLibrary::Prefix() {
    return "";
}

std::string SharedLibrary::Suffix() {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
    return "d.dll";
#else
    return ".dll";
#endif
}

void *SharedLibrary::FindSymbol(const std::string &name) {
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
