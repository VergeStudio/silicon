#include "silicon/network_impl_includes.hpp"


#ifdef SILICON_FEATURE_TLS


namespace silicon::network::tls {

static std::string send_status_ok{"ok"};
static std::string send_status_buffer_is_empty{"buffer_is_empty"};
static std::string send_status_timeout{"timeout"};
static std::string send_status_closed{"closed"};
static std::string send_status_error{"error"};
static std::string send_status_cancelled{"cancelled"};
static std::string send_status_want_read{"want_read"};
static std::string send_status_want_write{"want_write"};
static std::string send_status_want_connect{"want_connect"};
static std::string send_status_want_accept{"want_accept"};
static std::string send_status_want_x509_lookup{"want_x509_lookup"};
static std::string send_status_error_syscall{"error_syscall"};
static std::string send_status_unknown{"unknown"};

auto to_string(send_status status) -> const std::string & {
    switch(status) {
        case send_status::kOk:
            return send_status_ok;
        case send_status::kBufferIsEmpty:
            return send_status_buffer_is_empty;
        case send_status::kTimeout:
            return send_status_timeout;
        case send_status::kClosed:
            return send_status_closed;
        case send_status::kError:
            return send_status_error;
        case send_status::kCancelled:
            return send_status_cancelled;
        case send_status::kWantRead:
            return send_status_want_read;
        case send_status::kWantWrite:
            return send_status_want_write;
        case send_status::kWantConnect:
            return send_status_want_connect;
        case send_status::kWantAccept:
            return send_status_want_accept;
        case send_status::kWantX509Lookup:
            return send_status_want_x509_lookup;
        case send_status::kErrorSyscall:
            return send_status_error_syscall;
    }

    return send_status_unknown;
}

} // namespace silicon::network::tls

#endif // #ifdef SILICON_FEATURE_TLS
