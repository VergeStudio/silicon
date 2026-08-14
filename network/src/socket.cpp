// Implementation unit for silicon::network (socket + make_socket factories).
//
// Provides out-of-line member definitions for silicon::network::socket and the
// free make_socket / make_accept_socket factories. Network types come from the
// :core partition (implicit primary import); platform socket APIs come from
// the global module fragment below.

module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

#include <cerrno>
#include <expected>
#include <system_error>

module silicon.network;

import silicon.coroutine;

#ifdef _WIN32
// Winsock uses SD_RECEIVE/SD_SEND/SD_BOTH instead of the POSIX SHUT_RD/WR/RDWR.
#    ifndef SHUT_RD
#        define SHUT_RD SD_RECEIVE
#        define SHUT_WR SD_SEND
#        define SHUT_RDWR SD_BOTH
#    endif
#endif

namespace silicon::network {
auto socket::type_to_os(type_t type) -> result<int> {
    switch(type) {
        case type_t::udp:
            return SOCK_DGRAM;
        case type_t::tcp:
            return SOCK_STREAM;
    }
    return std::unexpected(make_error_code(network_error::kInvalidSocketType));
}

auto socket::operator=(const socket &other) noexcept -> socket & {
    this->close();
#ifdef _WIN32
    // Windows has no dup() for SOCKET handles; shallow-copy the handle.
    this->m_fd = other.m_fd;
#else
    this->m_fd = dup(other.m_fd);
#endif
    return *this;
}

auto socket::operator=(socket &&other) noexcept -> socket & {
    if(std::addressof(other) != this) {
        m_fd = std::exchange(other.m_fd, -1);
    }

    return *this;
}

auto socket::blocking(blocking_t block) -> bool {
    if(m_fd < 0) {
        return false;
    }

#ifdef _WIN32
    // Windows has no fcntl; non-blocking mode is controlled via ioctlsocket(FIONBIO).
    unsigned long mode = (block == blocking_t::yes) ? 0u : 1u;
    return (ioctlsocket(m_fd, FIONBIO, &mode) == 0);
#else
    int flags = fcntl(m_fd, F_GETFL, 0);
    if(flags == -1) {
        return false;
    }

    // Add or subtract non-blocking flag.
    flags = (block == blocking_t::yes) ? flags & ~O_NONBLOCK : (flags | O_NONBLOCK);

    return (fcntl(m_fd, F_SETFL, flags) == 0);
#endif
}

auto socket::shutdown(silicon::coroutine::poll_op how) -> bool {
    if(m_fd != -1) {
        int h{0};
        switch(how) {
            case silicon::coroutine::poll_op::read:
                h = SHUT_RD;
                break;
            case silicon::coroutine::poll_op::write:
                h = SHUT_WR;
                break;
            case silicon::coroutine::poll_op::read_write:
                h = SHUT_RDWR;
                break;
        }

        return (::shutdown(m_fd, h) == 0);
    }
    return false;
}

auto socket::close() -> void {
    if(m_fd != -1) {
#ifdef _WIN32
        ::closesocket(m_fd);
#else
        ::close(m_fd);
#endif
        m_fd = -1;
    }
}

auto make_socket(const socket::options &opts, domain_t domain) -> result<socket> {
    auto os_type = socket::type_to_os(opts.type);
    if(!os_type) { return std::unexpected(os_type.error()); }

    // On Windows ::socket() returns a SOCKET (unsigned 64-bit); the fd-based
    // design stores it as int, so an explicit cast is required (and matches the
    // existing i_socket::native_handle() -> int contract). INVALID_SOCKET maps to -1.
    socket s{static_cast<int>(::socket(static_cast<int>(domain), *os_type, 0))};
    if(s.native_handle() < 0) {
        return std::unexpected(make_error_code(network_error::kSocketCreateFailed));
    }

    if(opts.blocking == socket::blocking_t::no) {
        if(s.blocking(socket::blocking_t::no) == false) {
            return std::unexpected(make_error_code(network_error::kSetNonblockingFailed));
        }
    }

    return s;
}

auto make_accept_socket(const socket::options &opts, const network::socket_address &endpoint, int32_t backlog)
        -> result<socket> {
    auto domain = endpoint.domain();
    if(!domain) { return std::unexpected(domain.error()); }

    auto created = make_socket(opts, *domain);
    if(!created) { return std::unexpected(created.error()); }
    socket s = std::move(*created);

    [[maybe_unused]] int sock_opt{1};

#if defined(SILICON_PLATFORM_LINUX)
    // On Linux the address and port should be marked for reuse.
    if(setsockopt(s.native_handle(), SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt)) < 0) {
        return std::unexpected(make_error_code(network_error::kSetSockOptFailed));
    }
#endif

#if !defined(SILICON_PLATFORM_WINDOWS)
    // SO_REUSEPORT is a BSD/Linux socket option; Windows has no equivalent
    // (SO_REUSEADDR already covers the port-reuse semantics there).
    if(setsockopt(s.native_handle(), SOL_SOCKET, SO_REUSEPORT, &sock_opt, static_cast<int>(sizeof(sock_opt))) < 0) {
        return std::unexpected(make_error_code(network_error::kSetSockOptFailed));
    }
#endif

    auto [sockaddr, socklen] = endpoint.data();

    if(bind(s.native_handle(), sockaddr, socklen) < 0) {
        return std::unexpected(make_error_code(network_error::kBindFailed));
    }

    if(opts.type == socket::type_t::tcp) {
        if(listen(s.native_handle(), backlog) < 0) {
            return std::unexpected(make_error_code(network_error::kListenFailed));
        }
    }

    return s;
}

auto socket::accept(socket_address &client_endpoint) -> socket {
    auto [addr, addrlen] = client_endpoint.data();

#ifdef _WIN32
    // Winsock's accept() takes an int* for addrlen and returns a SOCKET handle.
    int len = static_cast<int>(addrlen);
    auto raw = ::accept(m_fd, const_cast<sockaddr *>(addr), &len);
    return socket{static_cast<int>(raw)};
#else
    auto raw = ::accept(m_fd, const_cast<sockaddr *>(addr), addrlen);
    return socket{raw};
#endif
}

auto socket::last_error() const -> int {
#ifdef _WIN32
    return static_cast<int>(WSAGetLastError());
#else
    return errno;
#endif
}

auto socket::connect(const socket_address &endpoint) -> int {
    auto [addr, addrlen] = endpoint.data();

#ifdef _WIN32
    // Winsock's connect() takes an int for addrlen and returns SOCKET_ERROR (-1)
    // on failure (check last_error() == WSAEWOULDBLOCK for async in-progress).
    int len = static_cast<int>(addrlen);
    return static_cast<int>(::connect(m_fd, const_cast<sockaddr *>(addr), len));
#else
    return ::connect(m_fd, const_cast<sockaddr *>(addr), addrlen);
#endif
}

auto socket::in_progress() const -> bool {
    // A non-blocking connect() that has not yet completed returns EINPROGRESS
    // on POSIX or WSAEWOULDBLOCK on Windows; either way the connection is
    // establishing asynchronously and the caller should poll for writability.
#ifdef _WIN32
    return (last_error() == WSAEWOULDBLOCK);
#else
    return (last_error() == EINPROGRESS);
#endif
}

} // namespace silicon::network
