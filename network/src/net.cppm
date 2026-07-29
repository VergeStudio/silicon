module;

#include "silicon/network/connect.hpp"
#include "silicon/network/hostname.hpp"
#include "silicon/network/io_status.hpp"
#include "silicon/network/ip_address.hpp"
#include "silicon/network/recv_status.hpp"
#include "silicon/network/send_status.hpp"
#include "silicon/network/socket.hpp"
#include "silicon/network/socket_address.hpp"
#include "silicon/network/dns/resolver.hpp"
#include "silicon/network/tcp/client.hpp"
#include "silicon/network/tcp/server.hpp"
#include "silicon/network/udp/peer.hpp"
#ifdef LIBCORO_FEATURE_TLS
#    include "silicon/network/tls/client.hpp"
#    include "silicon/network/tls/server.hpp"
#    include "silicon/network/tls/context.hpp"
#    include "silicon/network/tls/connection_status.hpp"
#    include "silicon/network/tls/recv_status.hpp"
#    include "silicon/network/tls/send_status.hpp"
#endif

export module silicon.network;

export import :config;

// Re-export public types via using-declarations for module consumers.

export namespace silicon::coroutine::network {
    using ::silicon::coroutine::network::connect;
    using ::silicon::coroutine::network::hostname;
    using ::silicon::coroutine::network::io_status;
    using ::silicon::coroutine::network::ip_address;
    using ::silicon::coroutine::network::recv_status;
    using ::silicon::coroutine::network::send_status;
    using ::silicon::coroutine::network::socket;
    using ::silicon::coroutine::network::socket_address;
    using ::silicon::coroutine::network::dns::resolver;
    using ::silicon::coroutine::network::tcp::client;
    using ::silicon::coroutine::network::tcp::server;
    using ::silicon::coroutine::network::udp::peer;
#ifdef LIBCORO_FEATURE_TLS
    using ::silicon::coroutine::network::tls::client;
    using ::silicon::coroutine::network::tls::server;
    using ::silicon::coroutine::network::tls::context;
    using ::silicon::coroutine::network::tls::connection_status;
    using ::silicon::coroutine::network::tls::recv_status;
    using ::silicon::coroutine::network::tls::send_status;
#endif
} // namespace silicon::coroutine::network
