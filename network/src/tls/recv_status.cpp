#include "silicon/network_impl_includes.hpp"


#ifdef SILICON_FEATURE_TLS


namespace silicon::network::tls {

static std::string recv_status_ok{"ok"};
static std::string recv_status_buffer_is_empty{"buffer_is_empty"};
static std::string recv_status_timeout{"timeout"};
static std::string recv_status_closed{"closed"};
static std::string recv_status_error{"error"};
static std::string recv_status_cancelled{"cancelled"};
static std::string recv_status_want_read{"want_read"};
static std::string recv_status_want_write{"want_write"};
static std::string recv_status_want_connect{"want_connect"};
static std::string recv_status_want_accept{"want_accept"};
static std::string recv_status_want_x509_lookup{"want_x509_lookup"};
static std::string recv_status_error_syscall{"error_syscall"};
static std::string recv_status_unknown{"unknown"};

auto to_string(recv_status status) -> const std::string & {
    switch(status) {
        case recv_status::kOk:
            return recv_status_ok;
        case recv_status::kBufferIsEmpty:
            return recv_status_buffer_is_empty;
        case recv_status::kTimeout:
            return recv_status_timeout;
        case recv_status::kClosed:
            return recv_status_closed;
        case recv_status::kError:
            return recv_status_error;
        case recv_status::kCancelled:
            return recv_status_cancelled;
        case recv_status::kWantRead:
            return recv_status_want_read;
        case recv_status::kWantWrite:
            return recv_status_want_write;
        case recv_status::kWantConnect:
            return recv_status_want_connect;
        case recv_status::kWantAccept:
            return recv_status_want_accept;
        case recv_status::kWantX509Lookup:
            return recv_status_want_x509_lookup;
        case recv_status::kErrorSyscall:
            return recv_status_error_syscall;
    }

    return recv_status_unknown;
}

} // namespace silicon::network::tls

#endif // #ifdef SILICON_FEATURE_TLS
