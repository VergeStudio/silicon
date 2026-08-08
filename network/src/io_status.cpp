// Implementation unit for silicon::network (io_status shared helpers).

module;

#include <string>
#include <string_view>

module silicon.network;

import silicon.coroutine;

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
            return io_status{io_status::kind::kOk};
        case silicon::coroutine::poll_status::timeout:
            return io_status{io_status::kind::kTimeout};
        case silicon::coroutine::poll_status::error:
            return io_status{io_status::kind::kPollingError};
        case silicon::coroutine::poll_status::closed:
            return io_status{io_status::kind::kClosed};
        case silicon::coroutine::poll_status::cancelled:
            return io_status{io_status::kind::kCancelled};
        default:
            return io_status{io_status::kind::kUnknown};
    }
}

// ── to_string (shared) ────────────────────────────────────────────
auto silicon::network::to_string(silicon::network::io_status::kind k) -> std::string_view {
    using kind = io_status::kind;
    switch(k) {
        case kind::kOk:
            return "Success";
        case kind::kClosed:
            return "Connection closed by peer";
        case kind::kConnectionReset:
            return "Connection reset by peer";
        case kind::kConnectionRefused:
            return "Connection refused by target host";
        case kind::kTimeout:
            return "Operation timed out";
        case kind::kWouldBlockOrTryAgain:
            return "Operation would block or try again";
        case kind::kPollingError:
            return "Polling error";
        case kind::kCancelled:
            return "Operation cancelled";
        case kind::kUdpNotBound:
            return "Udp socket is not bound";
        case kind::kNative:
            return "Native error code";
        case kind::kMessageTooBig:
            return "Message is too big";
        case kind::kUnknown:
        default:
            return "unknown";
    }
}
