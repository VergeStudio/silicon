module;

#if !defined(_WIN32) && !defined(_WIN64)
#    include <dlfcn.h>
#endif

#include <mutex>
#include <string>

module silicon.library;

import silicon.platform;
import silicon.exception;

namespace silicon::library {

SharedLibrary::SharedLibrary() {
}

void SharedLibrary::Load(const std::string &rPath, int32_t flags) {
    std::scoped_lock<std::mutex> const lock(m_mutex);

    if (m_pHandle != nullptr) {
        throw RuntimeError("Library already loaded: " + rPath);
    }

    m_pHandle = dlopen(rPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (m_pHandle == nullptr) {
        const char *err = dlerror();
        throw RuntimeError("Could not load library: " + (err ? std::string(err) : rPath));
    }
    m_path = rPath;
}

void SharedLibrary::Unload() {
    std::scoped_lock<std::mutex> const lock(m_mutex);

    if (m_pHandle != nullptr) {
        dlclose(m_pHandle);
        m_pHandle = nullptr;
    }
}

bool SharedLibrary::IsLoaded() const {
    return m_pHandle != nullptr;
}

const std::string &SharedLibrary::GetPath() const {
    return m_path;
}

std::string SharedLibrary::Prefix() {
    if constexpr (os == os_id::cygwin) {
        return "cyg";
    } else {
        return "lib";
    }
}

std::string SharedLibrary::Suffix() {
    if constexpr (os == os_id::mac_os_x) {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
        return "d.dylib";
#else
        return ".dylib";
#endif
    } else if constexpr (os == os_id::hpux) {
#if defined(_DEBUG) && !defined(CL_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
        return "d.sl";
#else
        return ".sl";
#endif
    } else if constexpr (os == os_id::cygwin) {
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

void *SharedLibrary::findSymbol(const std::string &rName) {
    std::scoped_lock<std::mutex> const lock(m_mutex);

    void *result = nullptr;
    if (m_pHandle) {
        result = dlsym(m_pHandle, rName.c_str());
    }
    return result;
}

} // namespace silicon::library
