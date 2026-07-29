#pragma once

#if defined(_WIN32) || defined(_WIN64)
#    include <io.h>
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <unistd.h>
#endif

#include <iostream>
#include <span>
#include <utility>

import silicon.coroutine;
#include "silicon/network/isocket.hpp"
#include "silicon/network/socket_address.hpp"

namespace silicon::network {
class socket final: public ISocket {
  public:
    enum class type_t {
        /// udp datagram socket
        udp,
        /// tcp streaming socket
        tcp
    };

    enum class blocking_t {
        /// This socket should block on system calls.
        yes,
        /// This socket should not block on system calls.
        no
    };

    struct options {
        /// The type of socket.
        type_t type;
        /// If the socket should be blocking or non-blocking.
        blocking_t blocking;
    };

    static auto type_to_os(type_t type) -> int;

    socket() = default;
    explicit socket(int fd): m_fd(fd) {}

#ifdef _WIN32
    // Windows has no dup() for SOCKET handles; shallow-copy the handle.
    socket(const socket &other): m_fd(other.m_fd) {}
#else
    socket(const socket &other): m_fd(dup(other.m_fd)) {}
#endif
    socket(socket &&other) noexcept: m_fd(std::exchange(other.m_fd, -1)) {}
    auto operator=(const socket &other) noexcept -> socket &;
    auto operator=(socket &&other) noexcept -> socket &;

    ~socket() override { close(); }

    /**
     * This function returns true if the socket's file descriptor is a valid number, however it does
     * not imply if the socket is still usable.
     * @return True if the socket file descriptor is > 0.
     */
    [[nodiscard]] auto is_ok() const -> bool override { return m_fd != -1; }

    explicit operator bool() const { return is_ok(); }

    /**
     * @param block Sets the socket to the given blocking mode.
     */
    auto blocking(blocking_t block) -> bool;
    auto blocking(int block) -> bool override { return blocking(static_cast<blocking_t>(block)); }

    /**
     * @param how Shuts the socket down with the given operations.
     * @return Returns true if the sockets given operations were shutdown.
     */
    auto shutdown(silicon::coroutine::poll_op how = silicon::coroutine::poll_op::read_write) -> bool;
    auto shutdown(int how) -> bool override { return shutdown(static_cast<silicon::coroutine::poll_op>(how)); }

    /**
     * Closes the socket and sets this socket to an invalid state.
     */
    auto close() -> void override;

    /**
     * @return The native handle (file descriptor) for this socket.
     */
    auto native_handle() const -> int override { return m_fd; }

    /**
     * Accepts a pending incoming connection on a listening (accept) socket.
     * @param client_endpoint Receives the address of the connected peer.
     * @return The newly accepted socket. Check is_ok() to detect failure.
     */
    auto accept(socket_address &client_endpoint) -> socket;

    /**
     * @return The last platform-specific socket error code for this socket
     *         (errno on POSIX, WSAGetLastError() on Windows).
     */
    auto last_error() const -> int;

    /**
     * Initiates a connection on this socket to the given endpoint. On a non-blocking socket
     * the connection is typically established in the background; use in_progress() to detect
     * that case.
     * @param endpoint The remote address to connect to.
     * @return 0 if the connection completed immediately, non-zero otherwise (check in_progress()).
     */
    auto connect(const socket_address &endpoint) -> int;

    /**
     * @return True if the most recent connect() is still being established asynchronously
     *         (EINPROGRESS on POSIX, WSAEWOULDBLOCK on Windows).
     */
    auto in_progress() const -> bool;

  private:
    int m_fd{-1};
};

/**
 * Creates a socket with the given socket options, this typically is used for creating sockets to
 * use within client objects, e.g. tcp::client and udp::client.
 * @param opts See socket::options for more details.
 * TODO: docs
 */
auto make_socket(const socket::options &opts, domain_t) -> socket;

/**
 * Creates a socket that can accept connections or packets with the given socket options, address,
 * port and backlog.  This is used for creating sockets to use within server objects, e.g.
 * tcp::server and udp::server.
 * @param opts See socket::options for more details
 * @param address The ip address to bind to.  If the type of socket is tcp then it will also listen.
 * @param port The port to bind to.
 * @param backlog If the type of socket is tcp then the backlog of connections to allow.  Does nothing
 *                for udp types.
 * TODO: docs
 */
auto make_accept_socket(const socket::options &opts, const network::socket_address &endpoint, int32_t backlog) -> socket;

} // namespace silicon::network
