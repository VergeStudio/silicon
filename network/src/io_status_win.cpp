// Windows implementation of platform-specific io_status helpers.

#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "silicon/network_impl_includes.hpp"

namespace silicon::network {

std::string message_impl(int native_code) {
    char *buffer = nullptr;
    size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            static_cast<DWORD>(native_code),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&buffer),
            0,
            nullptr
    );

    if(size > 0 && buffer) {
        std::string msg(buffer, size);
        LocalFree(buffer);
        return msg;
    }
    return "Unknown Windows error (" + std::to_string(native_code) + ")";
}

auto make_io_status_from_native_impl(int native_code) -> io_status {
    // TODO: map Windows error codes to io_status::kind values
    return io_status{.type = io_status::kind::kNative, .native_code = native_code};
}

} // namespace silicon::network
