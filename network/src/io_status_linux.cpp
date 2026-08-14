// Linux/BSD implementation of platform-specific io_status helpers.
// 平台无关的 io_status 公共实现（message()/make_io_status_from_native/
// make_io_status_from_poll_status/to_string）在公共实现单元 io_status.cpp；
// 本文件仅提供平台差异的 message_impl / make_io_status_from_native_impl。
// 守卫与 io_status_win.cpp 的 SILICON_PLATFORM_WINDOWS 守卫互斥，恰好一个文件定义同组符号。

module;

#include <cerrno>
#include <errno.h>
#include <string>
#include <system_error>

module silicon.network;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::network {

std::string message_impl(int native_code) {
    // Linux: use std::system_category for system error messages
    try {
        return std::system_category().message(native_code);
    } catch(...) {
        return "Unknown system error (" + std::to_string(native_code) + ")";
    }
}

auto make_io_status_from_native_impl(int native_code) -> io_status {
    using kind = io_status::kind;
    kind type;
    switch(native_code) {
        case 0:
            type = kind::kOk;
            break;
        case EOF:
            type = kind::kClosed;
            break;
        case ECONNREFUSED:
            type = kind::kConnectionRefused;
            break;
        case ECONNRESET:
            type = kind::kConnectionReset;
            break;
        case EAGAIN:
#    if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#    endif
            type = kind::kWouldBlockOrTryAgain;
            break;
        case EMSGSIZE:
            type = kind::kMessageTooBig;
            break;
        default:
            type = kind::kNative;
            break;
    }
    return io_status{.type = type, .native_code = native_code};
}

} // namespace silicon::network

#endif // defined(SILICON_PLATFORM_UNIX)
