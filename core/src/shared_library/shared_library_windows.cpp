module;

#if defined(_WIN32) || defined(_WIN64)
#    include <Windows.h>
#endif

#include <mutex>
#include <string>

module silicon.library;

import silicon.platform;
import silicon.exception;

#if defined(_WIN32) || defined(_WIN64)

#include "libloaderapi.h"
#include "minwindef.h"

namespace silicon::library {

SharedLibrary::SharedLibrary() {
}

void SharedLibrary::Load(const std::string &rPath, int32_t flags) {
    std::scoped_lock<std::mutex> const lock(m_mutex);

    m_pHandle = LoadLibrary(rPath.c_str());
    if (m_pHandle == nullptr) {
        throw silicon::exception::RuntimeError("Could not load library: " + rPath);
    }
    m_path = rPath;
}

void SharedLibrary::Unload() {
    std::scoped_lock<std::mutex> const lock(m_mutex);

    if (m_pHandle != nullptr) {
        FreeLibrary(static_cast<HMODULE>(m_pHandle));
        m_pHandle = nullptr;
    }
    m_path.clear();
}

bool SharedLibrary::IsLoaded() const {
    return m_pHandle != nullptr;
}

const std::string &SharedLibrary::GetPath() const {
    return m_path;
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

void *SharedLibrary::findSymbol(const std::string &rName) {
    std::scoped_lock<std::mutex> const lock(m_mutex);

    if (m_pHandle != nullptr) {
#if defined(_WIN32_WCE)
        std::wstring uname;
        UnicodeConverter::toUTF16(rName, uname);
        return static_cast<void *>(GetProcAddressW(static_cast<HMODULE>(m_pHandle), uname.c_str()));
#else
        // 函数指针 → 对象指针的 static_cast 是 MS 扩展（-Wmicrosoft-cast），
        // 标准写法需经 reinterpret_cast。
        return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(m_pHandle), rName.data()));
#endif
    }

    return nullptr;
}

} // namespace silicon::library

#endif // defined(_WIN32) || defined(_WIN64)
