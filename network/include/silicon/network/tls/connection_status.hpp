#ifdef SILICON_FEATURE_TLS

#    pragma once

#    include <string>

namespace silicon::network::tls {
enum class connection_status {
    /// The tls connection was successful.
    kConnected,
    /// The connection hasn't been established yet, use connect() prior to the handshake().
    kNotConnected,
    /// The connection needs a silicon::network::tls::context to perform the handshake.
    kContextRequired,
    /// The internal ssl memory alocation failed.
    kResourceAllocationFailed,
    /// Attempting to set the connections ssl socket/file descriptor failed.
    kSetFdFailure,
    /// The handshake had an error.
    kHandshakeFailed,
    /// The connection timed out.
    kTimeout,
    /// An error occurred while polling for read or write operations on the socket.
    kPollError,
    /// The socket was unexpectedly closed while attempting the handshake.
    kUnexpectedClose,
    /// The given ip address could not be parsed or is invalid.
    kInvalidIpAddress,
    /// There was an unrecoverable error, use errno to get more information on the specific error.
    kError
};

auto to_string(connection_status status) -> const std::string &;

} // namespace silicon::network::tls

#endif // #ifdef SILICON_FEATURE_TLS
