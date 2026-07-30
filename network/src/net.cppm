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

export namespace silicon::network {
    using ::silicon::network::connect;
    using ::silicon::network::hostname;
    using ::silicon::network::io_status;
    using ::silicon::network::ip_address;
    using ::silicon::network::recv_status;
    using ::silicon::network::send_status;
    using ::silicon::network::socket;
    using ::silicon::network::socket_address;
    using ::silicon::network::dns::resolver;
    using ::silicon::network::tcp::client;
    using ::silicon::network::tcp::server;
    using ::silicon::network::udp::peer;
#ifdef LIBCORO_FEATURE_TLS
    using ::silicon::network::tls::client;
    using ::silicon::network::tls::server;
    using ::silicon::network::tls::context;
    using ::silicon::network::tls::connection_status;
    using ::silicon::network::tls::recv_status;
    using ::silicon::network::tls::send_status;
#endif
} // namespace silicon::network
