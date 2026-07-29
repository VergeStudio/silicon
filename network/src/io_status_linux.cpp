// Linux/BSD implementation of platform-specific io_status helpers.
#include <system_error>

#include "silicon/network_impl_includes.hpp"

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
            type = kind::ok;
            break;
        case EOF:
            type = kind::closed;
            break;
        case ECONNREFUSED:
            type = kind::connection_refused;
            break;
        case ECONNRESET:
            type = kind::connection_reset;
            break;
        case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
            type = kind::would_block_or_try_again;
            break;
        case EMSGSIZE:
            type = kind::message_too_big;
            break;
        default:
            type = kind::native;
            break;
    }
    return io_status{.type = type, .native_code = native_code};
}

} // namespace silicon::network
