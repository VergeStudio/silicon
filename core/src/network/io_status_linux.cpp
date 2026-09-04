





module;

#include <cerrno>
#include <errno.h>
#include <string>
#include <system_error>
#include <coroutine>

module silicon.network;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::network {

std::string message_impl(int native_code) {

    try {
        return std::system_category().message(native_code);
    } catch(...) {
        return "Unknown system error (" + std::to_string(native_code) + ")";
    }
}

io_status make_io_status_from_native_impl(int native_code) {
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

}

#endif
