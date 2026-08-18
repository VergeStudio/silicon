// Windows implementation of platform-specific io_status helpers.
// 平台无关的 io_status 公共实现（message()/make_io_status_from_native/
// make_io_status_from_poll_status/to_string）在公共实现单元 io_status.cpp；
// 本文件仅提供平台差异的 message_impl / make_io_status_from_native_impl。
// 守卫与 io_status_linux.cpp 的 unix 系守卫互斥，恰好一个文件定义同组符号。

module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <windows.h>
#endif

#include <string>
#include <coroutine>

module silicon.network;

#if defined(SILICON_PLATFORM_WINDOWS)

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

#endif // defined(SILICON_PLATFORM_WINDOWS)
