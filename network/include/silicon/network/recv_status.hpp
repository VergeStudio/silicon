#pragma once

#include <errno.h>

#include <cstdint>
#include <string>

namespace silicon::network {

enum class recv_status : int64_t {
    kOk = 0,
    /// The peer closed the socket.
    kClosed = -1,
    /// The udp socket has not been bind()'ed to a local port.
    kUdpNotBound = -2,
    kTryAgain = EAGAIN,
    // Note: that only the tcp::client will return this, a tls::client returns the specific ssl_would_block_* status'.
    kWouldBlock = EWOULDBLOCK,
    kBadFileDescriptor = EBADF,
    kConnectionRefused = ECONNREFUSED,
    kMemoryFault = EFAULT,
    kInterrupted = EINTR,
    kInvalidArgument = EINVAL,
    kNoMemory = ENOMEM,
    kNotConnected = ENOTCONN,
    kNotASocket = ENOTSOCK,
    kConnectionResetByPeer = ECONNRESET,
};

auto to_string(recv_status status) -> const std::string &;

} // namespace silicon::network
