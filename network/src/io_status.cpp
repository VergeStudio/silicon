#include "silicon/network_impl_includes.hpp"

// ── Shared: message() preamble (same on all platforms) ──────────────────
std::string silicon::network::io_status::message() const {
    if(not is_native()) {
        return std::string{to_string(type)};
    }

    if(native_code == 0) {
        return "Success";
    }

    // Platform-specific error message retrieval
    return message_impl(native_code);
}

// ── make_io_status_from_native (platform dispatch) ────────────────────
auto silicon::network::make_io_status_from_native(int native_code) -> silicon::network::io_status {
    return make_io_status_from_native_impl(native_code);
}

// ── make_io_status_from_poll_status (shared — no platform deps) ───────
auto silicon::network::make_io_status_from_poll_status(silicon::coroutine::poll_status status) -> silicon::network::io_status {
    switch(status) {
        case silicon::coroutine::poll_status::read:
        case silicon::coroutine::poll_status::write:
            return io_status{io_status::kind::ok};
        case silicon::coroutine::poll_status::timeout:
            return io_status{io_status::kind::timeout};
        case silicon::coroutine::poll_status::error:
            return io_status{io_status::kind::polling_error};
        case silicon::coroutine::poll_status::closed:
            return io_status{io_status::kind::closed};
        case silicon::coroutine::poll_status::cancelled:
            return io_status{io_status::kind::cancelled};
        default:
            return io_status{io_status::kind::unknown};
    }
}

// ── to_string (shared) ────────────────────────────────────────────
auto silicon::network::to_string(silicon::network::io_status::kind k) -> std::string_view {
    using kind = io_status::kind;
    switch(k) {
        case kind::ok:
            return "Success";
        case kind::closed:
            return "Connection closed by peer";
        case kind::connection_reset:
            return "Connection reset by peer";
        case kind::connection_refused:
            return "Connection refused by target host";
        case kind::timeout:
            return "Operation timed out";
        case kind::would_block_or_try_again:
            return "Operation would block or try again";
        case kind::polling_error:
            return "Polling error";
        case kind::cancelled:
            return "Operation cancelled";
        case kind::udp_not_bound:
            return "Udp socket is not bound";
        case kind::native:
            return "Native error code";
        case kind::message_too_big:
            return "Message is too big";
        case kind::unknown:
        default:
            return "unknown";
    }
}
