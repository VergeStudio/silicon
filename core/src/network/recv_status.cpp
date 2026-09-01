// Implementation unit for silicon::network (recv_status to_string).

module;

#include <string>
#include <coroutine>

module silicon.network;

namespace silicon::network {

static const std::string recv_status_ok{"ok"};
static const std::string recv_status_closed{"closed"};
static const std::string recv_status_udp_not_bound{"udp_not_bound"};
static const std::string recv_status_would_block{"would_block"};
static const std::string recv_status_bad_file_descriptor{"bad_file_descriptor"};
static const std::string recv_status_connection_refused{"connection_refused"};
static const std::string recv_status_memory_fault{"memory_fault"};
static const std::string recv_status_interrupted{"interrupted"};
static const std::string recv_status_invalid_argument{"invalid_argument"};
static const std::string recv_status_no_memory{"no_memory"};
static const std::string recv_status_not_connected{"not_connected"};
static const std::string recv_status_not_a_socket{"not_a_socket"};
static const std::string connection_reset_by_peer{"connection_reset_by_peer"};
static const std::string recv_status_try_again{"try_again"};
static const std::string recv_status_unknown{"unknown"};

auto to_string(recv_status status) -> const std::string & {
    switch(status) {
        case recv_status::kOk:
            return recv_status_ok;
        case recv_status::kClosed:
            return recv_status_closed;
        case recv_status::kUdpNotBound:
            return recv_status_udp_not_bound;
// BSD/macOS 上 EAGAIN 与 EWOULDBLOCK 同值（均为 35），枚举值重复会导致
// switch 重复 case。二者不同值的平台才需要该分支（本单元不引入 <cerrno>，
// 故宏未定义时同样跳过——此时 EAGAIN/EWOULDBLOCK 必同值）。
#if defined(EWOULDBLOCK) && defined(EAGAIN) && (EWOULDBLOCK != EAGAIN)
        case recv_status::kWouldBlock:
            return recv_status_would_block;
#endif
        case recv_status::kTryAgain:
            return recv_status_try_again;
        case recv_status::kBadFileDescriptor:
            return recv_status_bad_file_descriptor;
        case recv_status::kConnectionRefused:
            return recv_status_connection_refused;
        case recv_status::kMemoryFault:
            return recv_status_memory_fault;
        case recv_status::kInterrupted:
            return recv_status_interrupted;
        case recv_status::kInvalidArgument:
            return recv_status_invalid_argument;
        case recv_status::kNoMemory:
            return recv_status_no_memory;
        case recv_status::kNotConnected:
            return recv_status_not_connected;
        case recv_status::kNotASocket:
            return recv_status_not_a_socket;
        case recv_status::kConnectionResetByPeer:
            return connection_reset_by_peer;
    }

    return recv_status_unknown;
}

} // namespace silicon::network
