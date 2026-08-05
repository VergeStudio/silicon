#ifdef SILICON_FEATURE_TLS

#    pragma once

#    include <errno.h>
#    include <openssl/ssl.h>

#    include <cstdint>
#    include <string>

namespace silicon::network::tls {

enum class send_status : int64_t {
    kOk = SSL_ERROR_NONE,
    // The user provided an 0 length buffer.
    kBufferIsEmpty = -3,
    // The operation timed out.
    kTimeout = -4,
    /// The operation was cancelled.
    kCancelled = -5,
    /// The peer closed the socket.
    kClosed = SSL_ERROR_ZERO_RETURN,
    kError = SSL_ERROR_SSL,
    kWantRead = SSL_ERROR_WANT_READ,
    kWantWrite = SSL_ERROR_WANT_WRITE,
    kWantConnect = SSL_ERROR_WANT_CONNECT,
    kWantAccept = SSL_ERROR_WANT_ACCEPT,
    kWantX509Lookup = SSL_ERROR_WANT_X509_LOOKUP,
    kErrorSyscall = SSL_ERROR_SYSCALL,

};

auto to_string(send_status status) -> const std::string &;

} // namespace silicon::network::tls

#endif // #ifdef SILICON_FEATURE_TLS
