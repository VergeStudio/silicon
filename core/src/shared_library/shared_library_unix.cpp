module;
#include <memory>

#if !defined(_WIN32) && !defined(_WIN64)
#    include <dlfcn.h>
#endif

#include <mutex>
#include <string>

module silicon.library;

import silicon.platform;
import silicon.exception;

#include "shared_library_impl.hpp"

namespace silicon::library {

shared_library::shared_library() : impl_(std::make_unique<Impl>()) {
}

void shared_library::load(const std::string &path, int32_t flags) {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    if (impl_->handle_ != nullptr) {
        throw runtime_error("Library already loaded: " + path);
    }

    impl_->handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (impl_->handle_ == nullptr) {
        const char *err = dlerror();
        throw runtime_error("Could not load library: " + (err ? std::string(err) : path));
    }
    impl_->path_ = path;
}

void shared_library::unload() {
    std::scoped_lock<std::mutex> const lock(impl_->mutex_);

    if (impl_->handle_ != nullptr) {
        dlclose(impl_->handle_);
        impl_->handle_ = nullptr;
    }
}

bool shared_library::is_loaded() const {
    return impl_->handle_ != nullptr;
}

const std::string &shared_library::get_path() const {
    return impl_->path_;
}

std::string shared_library::prefix() {
    if constexpr (os == os_id::kCygwin) {
        return "cyg";
    } else {
        return "lib";
    }
}

std::string shared_library::suffix() {
    if constexpr (os == os_id::kMacOsX) {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
        return "d.dylib";
#else
        return ".dylib";
#endif
    } else if constexpr (os == os_id::kHpux) {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
        return "d.sl";
#else
        return ".sl";
#endif
    } else if constexpr (os == os_id::kCygwin) {
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
