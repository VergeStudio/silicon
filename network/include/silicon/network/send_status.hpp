#pragma once

#include <errno.h>

#include <cstdint>

namespace silicon::network {

enum class send_status : int64_t {
    kOk = 0,
    kClosed = -1,
    kPermissionDenied = EACCES,
    kTryAgain = EAGAIN,
    kWouldBlock = EWOULDBLOCK,
    kAlreadyInProgress = EALREADY,
    kBadFileDescriptor = EBADF,
    kConnectionReset = ECONNRESET,
    kNoPeerAddress = EDESTADDRREQ,
    kMemoryFault = EFAULT,
    kInterrupted = EINTR,
    kIsConnection = EISCONN,
    kMessageSize = EMSGSIZE,
    kOutputQueueFull = ENOBUFS,
    kNoMemory = ENOMEM,
    kNotConnected = ENOTCONN,
    kNotASocket = ENOTSOCK,
    kOperationgNotSupported = EOPNOTSUPP,
    kPipeClosed = EPIPE,
};

} // namespace silicon::network
