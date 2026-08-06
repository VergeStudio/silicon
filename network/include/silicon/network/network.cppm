module;

#include "silicon/network/connect.hpp"
#include "silicon/network/dns/resolver.hpp"
#include "silicon/network/hostname.hpp"
#include "silicon/network/io_status.hpp"
#include "silicon/network/ip_address.hpp"
#include "silicon/network/recv_status.hpp"
#include "silicon/network/send_status.hpp"
#include "silicon/network/socket.hpp"
#include "silicon/network/socket_address.hpp"
#include "silicon/network/tcp/client.hpp"
#include "silicon/network/tcp/server.hpp"
#include "silicon/network/udp/peer.hpp"
#ifdef SILICON_FEATURE_TLS
#    include "silicon/network/tls/client.hpp"
#    include "silicon/network/tls/connection_status.hpp"
#    include "silicon/network/tls/context.hpp"
#    include "silicon/network/tls/recv_status.hpp"
#    include "silicon/network/tls/send_status.hpp"
#    include "silicon/network/tls/server.hpp"
#endif

export module silicon.network;

export import :config;

// 注意：此处刻意不通过 `using` 转发任何头文件类型（socket / io_status /
// domain_t / make_socket / ...）。C++20 命名模块无法导出位于头文件 global
// module fragment 中的声明，而实现单元（module silicon.network;）也无法通过
// `import` 看到接口单元 global fragment 里的类型。因此实现单元直接在自身的
// global module fragment 中 #include 这些头（由工具链包装脚本注入
// silicon/network_impl_includes.hpp），与非模块化 TU 行为一致。接口单元只
// 负责 `export import :config;`。
