#pragma once

// Internal implementation includes for the silicon::network library.
//
// Every translation unit under network/src/**.cpp includes this header
// first. It was missing from the leo/dev tree (referenced by all network
// .cpp files but never committed), which broke dependency scanning.
// Reconstructed here to unblock the build (mirrors
// coroutine_impl_includes.hpp).
//
// The header pulls in the full public network API (matching the network.cppm
// global module fragment) plus the coroutine types used by the impl and the
// platform system socket headers.
//
// NOTE: the network implementation files are POSIX-oriented (fcntl/dup/close/
// shutdown/bind/...). On Windows a winsock port is still required for the
// module to compile; this header only fixes the missing-include error.

#include "silicon/network/common.h"
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
#ifdef LIBCORO_FEATURE_TLS
#    include "silicon/network/tls/client.hpp"
#    include "silicon/network/tls/connection_status.hpp"
#    include "silicon/network/tls/context.hpp"
#    include "silicon/network/tls/recv_status.hpp"
#    include "silicon/network/tls/send_status.hpp"
#    include "silicon/network/tls/server.hpp"
#endif

// coroutine types used by the impl (e.g. silicon::coroutine::poll_op)
import silicon.coroutine;
// io_scheduler（原 silicon::coroutine::IScheduler）现属 silicon.scheduler
import silicon.scheduler;

// Platform system socket headers.
#if defined(_WIN32)
#    include <winsock2.h>
#    include <ws2tcpip.h>
#elif defined(__linux__)
#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#    include <unistd.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif
