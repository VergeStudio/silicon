// Interface partition silicon.network:core
//
// Holds the platform-independent core public API of silicon::network:
// connect_status, hostname, io_status, recv_status, send_status, ip_address,
// ISocket, socket_address and socket (plus the make_socket / make_accept_socket
// factories). All declarations are exported so consumers and the sibling
// partitions (:dns/:tcp/:udp/:tls) can name these types.
//
// Network types that reference silicon::coroutine scheduling primitives
// (poll_op / poll_status) pull them in via `import silicon.coroutine;`.

module;

#if defined(_WIN32) || defined(_WIN64)
#    include <io.h>
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

export module silicon.network:core;
export import silicon.network.error;

export import silicon.coroutine;

export namespace silicon::network {

/// 统一错误返回类型：std::expected<T, std::error_code> 的别名。
/// 错误码来源：silicon::network::network_error 枚举（make_error_code）或
/// silicon::network::system_error(errno)（POSIX errno / WSA 语义）。
template<typename T>
using result = std::expected<T, std::error_code>;


/// 从 POSIX errno 值构造 std::error_code（generic_category）。
[[nodiscard]] inline std::error_code system_error(int errno_value) noexcept {
    return {errno_value, std::generic_category()};
}

/// 从 std::errc 枚举构造 std::error_code（generic_category）。
[[nodiscard]] inline std::error_code system_error(std::errc e) noexcept {
    return std::make_error_code(e);
}


enum class connect_status {
    /// The connection has been established.
    kConnected,
    /// The given ip address could not be parsed or is invalid.
    kInvalidIpAddress,
    /// The connection operation timed out.
    kTimeout,
    /// There was an error, use errno to get more information on the specific error.
    kError
};

/**
 * @param status String representation of the connection status.
 * @return 字符串视图（指向静态存储）；枚举值非法时返回
 *         network_error::kInvalidConnectStatus。
 */
auto to_string(const connect_status &status) -> result<std::string_view>;

class hostname {
    struct Impl {
      public:
        std::string m_hostname;
    };
    std::shared_ptr<Impl> m_p{std::make_shared<Impl>()};

  public:
    hostname() = default;
    explicit hostname(std::string hn): m_p(std::make_shared<Impl>()) { m_p->m_hostname = std::move(hn); }
    // 值类型语义：拷贝做深拷贝，不与源对象共享实现
    hostname(const hostname &o): m_p(std::make_shared<Impl>(*o.m_p)) {}
    hostname(hostname &&) noexcept = default;
    auto operator=(const hostname &o) -> hostname & {
        if(this != &o) { m_p = std::make_shared<Impl>(*o.m_p); }
        return *this;
    }
    auto operator=(hostname &&) noexcept -> hostname & = default;
    ~hostname() = default;

    auto data() const -> const std::string & { return m_p->m_hostname; }

    auto operator<=>(const hostname &other) const { return m_p->m_hostname <=> other.m_p->m_hostname; }
    auto operator==(const hostname &other) const -> bool { return m_p->m_hostname == other.m_p->m_hostname; }

  private:
};

struct io_status {
    enum class kind {
        kOk,
        kClosed,
        kConnectionReset,
        kConnectionRefused,
        kTimeout,

        kWouldBlockOrTryAgain,
        kPollingError,
        kCancelled,

        kUdpNotBound,
        kMessageTooBig,

        kNative,

        kUnknown
    };

    kind type{};
    [[maybe_unused]] int native_code{};

    [[nodiscard]] auto is_ok() const -> bool { return type == kind::kOk; }
    [[nodiscard]] auto is_timeout() const -> bool { return type == kind::kTimeout; }
    [[nodiscard]] auto is_closed() const -> bool { return type == kind::kClosed; }
    [[nodiscard]] auto would_block() const -> bool { return type == kind::kWouldBlockOrTryAgain; }
    [[nodiscard]] auto try_again() const -> bool { return type == kind::kWouldBlockOrTryAgain; }

    [[nodiscard]] auto is_native() const -> bool { return type == kind::kNative; }

    explicit operator bool() const { return is_ok(); }

    /**
     * Returns a human-readable description of the error.
     */
    [[nodiscard]] auto message() const -> std::string;
};

auto to_string(io_status::kind kind) -> std::string_view;
auto make_io_status_from_native(int native_code) -> io_status;
auto make_io_status_from_poll_status(silicon::coroutine::poll_status status) -> io_status;

// ── Platform-specific helpers (defined in io_status_linux.cpp / io_status_win.cpp) ──
[[nodiscard]] std::string message_impl(int native_code);
auto make_io_status_from_native_impl(int native_code) -> io_status;

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

enum class domain_t : int {
    kIpv4 = AF_INET,
    kIpv6 = AF_INET6
};

/// @return 字符串视图（指向静态存储）；枚举值非法时返回
///         network_error::kInvalidDomain。
auto to_string(domain_t domain) -> result<std::string_view>;

class ip_address {
  public:
    static const constexpr size_t ipv4_len{4};
    static const constexpr size_t ipv6_len{16};

    ip_address() = default;

    /// 由二进制地址构造。长度超出对应域上限时返回
    /// network_error::kInvalidIpAddress。
    static auto from_binary(std::span<const uint8_t> binary_address,
                            domain_t domain = domain_t::kIpv4) -> result<ip_address> {
        if(domain == domain_t::kIpv4 && binary_address.size() > ipv4_len) {
            return std::unexpected(make_error_code(network_error::kInvalidIpAddress));
        }
        if(binary_address.size() > ipv6_len) {
            return std::unexpected(make_error_code(network_error::kInvalidIpAddress));
        }

        ip_address addr{};
        addr.m_p->m_domain = domain;
        std::copy(binary_address.begin(), binary_address.end(), addr.m_p->m_data.begin());
        return addr;
    }

    // 值类型语义：拷贝做深拷贝，不与源对象共享实现
    ip_address(const ip_address &o): m_p(std::make_shared<Impl>(*o.m_p)) {}
    ip_address(ip_address &&) noexcept = default;
    auto operator=(const ip_address &o) -> ip_address & {
        if(this != &o) { m_p = std::make_shared<Impl>(*o.m_p); }
        return *this;
    }
    auto operator=(ip_address &&) noexcept -> ip_address & = default;
    ~ip_address() = default;

    auto domain() const -> domain_t { return m_p->m_domain; }
    auto data() const -> std::span<const uint8_t> {
        if(m_p->m_domain == domain_t::kIpv4) {
            return std::span<const uint8_t>{m_p->m_data.data(), ipv4_len};
        } else {
            return std::span<const uint8_t>{m_p->m_data.data(), ipv6_len};
        }
    }

    /// 解析点分/冒号十六进制文本地址。解析失败返回
    /// network_error::kInvalidIpAddress。
    static auto from_string(std::string_view address, domain_t domain = domain_t::kIpv4) -> result<ip_address> {
        ip_address addr{};
        addr.m_p->m_domain = domain;

        auto success = inet_pton(static_cast<int>(addr.m_p->m_domain), address.data(), addr.m_p->m_data.data());
        if(success != 1) {
            return std::unexpected(make_error_code(network_error::kInvalidIpAddress));
        }

        return addr;
    }

    /// 转为文本表示。转换失败返回 system_error(errno)。
    auto to_string() const -> result<std::string> {
        std::string output;
        if(m_p->m_domain == domain_t::kIpv4) {
            output.resize(INET_ADDRSTRLEN, '\0');
        } else {
            output.resize(INET6_ADDRSTRLEN, '\0');
        }

        auto success = inet_ntop(static_cast<int>(m_p->m_domain), m_p->m_data.data(), output.data(), output.length());
        if(success == nullptr) {
            return std::unexpected(system_error(errno));
        }

        auto len = strnlen(success, output.length());
        output.resize(len);
        return output;
    }

    auto operator<=>(const ip_address &other) const {
        if(auto c = m_p->m_domain <=> other.m_p->m_domain; c != 0) return c;
        return m_p->m_data <=> other.m_p->m_data;
    }
    auto operator==(const ip_address &other) const -> bool {
        return m_p->m_domain == other.m_p->m_domain && m_p->m_data == other.m_p->m_data;
    }

  private:
    struct Impl {
      public:
        domain_t m_domain{domain_t::kIpv4};
        std::array<uint8_t, ipv6_len> m_data{};
    };
    std::shared_ptr<Impl> m_p{std::make_shared<Impl>()};
};

/// @brief Abstract interface for a network socket.
///
/// The concrete socket class (silicon::network::socket) implements this
/// interface. socket retains its value semantics (copy via dup, move, etc.)
/// and the virtual destructor is a small extra cost for a thin fd wrapper.
///
/// Usage in DI:
///   c.bind<ISocket>().to<socket>(di::in_unique);
class ISocket {
  public:
    ISocket() = default;
    ISocket(const ISocket &) = delete;
    ISocket(ISocket &&) = delete;
    auto operator=(const ISocket &) -> ISocket & = delete;
    auto operator=(ISocket &&) -> ISocket & = delete;

    virtual ~ISocket() = default;

    /// @brief Returns true if the socket's fd is valid.
    [[nodiscard]] virtual auto is_ok() const -> bool = 0;

    /// @brief Sets the socket to the given blocking mode.
    /// @param block blocking_t::yes or blocking_t::no
    /// @return true on success.
    virtual auto blocking(int block) -> bool = 0;

    /// @brief Shuts the socket down with the given operations.
    virtual auto shutdown(int how) -> bool = 0;

    /// @brief Closes the socket and sets it to an invalid state.
    virtual auto close() -> void = 0;

    /// @brief Returns the native handle (file descriptor).
    [[nodiscard]] virtual auto native_handle() const -> int = 0;
};

/**
 * Represents IP address and port.
 */
class socket_address {
    struct Impl {
      public:
        sockaddr_storage m_storage{};
        socklen_t m_len = sizeof(sockaddr_storage);
    };
    std::shared_ptr<Impl> m_p{std::make_shared<Impl>()};

  public:
    /// 由文本 ip + 端口构造。文本解析失败时返回
    /// network_error::kInvalidIpAddress。
    static auto create(std::string_view ip, std::uint16_t port,
                       domain_t domain = domain_t::kIpv4) -> result<socket_address> {
        auto addr = ip_address::from_string(ip, domain);
        if(!addr) { return std::unexpected(addr.error()); }
        return socket_address{*addr, port};
    }

    socket_address(const ip_address &ip, std::uint16_t port) {
        auto &storage = m_p->m_storage;
        auto &len = m_p->m_len;
        if(ip.domain() == domain_t::kIpv4) {
            auto *sin = reinterpret_cast<sockaddr_in *>(&storage);
            sin->sin_family = AF_INET;
            sin->sin_port = htons(port);

            // BSD-specific field, redundant for input
#    if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
            sin->sin_len = sizeof(sockaddr_in);
#    endif

            std::memcpy(&sin->sin_addr, ip.data().data(), sizeof(in_addr));
            len = sizeof(sockaddr_in);
        } else if(ip.domain() == domain_t::kIpv6) {
            auto *sin6 = reinterpret_cast<sockaddr_in6 *>(&storage);
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons(port);

            // BSD-specific field
#    if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
            sin6->sin6_len = sizeof(sockaddr_in6);
#    endif

            std::memcpy(&sin6->sin6_addr, ip.data().data(), sizeof(in6_addr));
            len = sizeof(sockaddr_in6);

            // TODO: link-local addresses
            // sin6->sin6_scope_id = ip.scope_id();
        }
    }

    // 值类型语义：拷贝做深拷贝，不与源对象共享实现
    socket_address(const socket_address &o): m_p(std::make_shared<Impl>(*o.m_p)) {}
    socket_address(socket_address &&) noexcept = default;
    auto operator=(const socket_address &o) -> socket_address & {
        if(this != &o) { m_p = std::make_shared<Impl>(*o.m_p); }
        return *this;
    }
    auto operator=(socket_address &&) noexcept -> socket_address & = default;
    ~socket_address() = default;

    /**
     * @brief Gets a pointer to underlying sockaddr structure.
     * Suitable for systemcalls like connect(), bind() or sendto().
     * @return A pair containing the const sockaddr pointer and its length.
     */
    [[nodiscard]] auto data() const & -> std::pair<const sockaddr *, socklen_t> {
        return {reinterpret_cast<const sockaddr *>(&m_p->m_storage), m_p->m_len};
    }

    /// Prevent usage on temporary objects to avoid dangling pointers.
    auto data() const && -> std::pair<const sockaddr *, socklen_t> = delete;

    /**
     * @brief Provides access to the storage for modification.
     * Suitable for system calls like accept() or recvfrom().
     * @return A pair containing the sockaddr pointer and a pointer to its length.
     * @see make_unitialised()
     */
    [[nodiscard]] auto native_mutable_data() & -> std::pair<sockaddr *, socklen_t *> {
        return {reinterpret_cast<sockaddr *>(&m_p->m_storage), &m_p->m_len};
    }

    /**
     * @brief Extracts the ip_address from the endpoint.
     * @return ip_address；地址族不受支持时返回 network_error::kInvalidDomain。
     */
    [[nodiscard]] auto ip() const -> result<ip_address> {
        if(m_p->m_storage.ss_family == AF_INET) {
            auto *sin = reinterpret_cast<const sockaddr_in *>(&m_p->m_storage);
            return ip_address::from_binary(
                    {reinterpret_cast<const uint8_t *>(&sin->sin_addr), sizeof(sin->sin_addr)}, domain_t::kIpv4
            );
        }
        if(m_p->m_storage.ss_family == AF_INET6) {
            auto *sin6 = reinterpret_cast<const sockaddr_in6 *>(&m_p->m_storage);
            return ip_address::from_binary(
                    {reinterpret_cast<const uint8_t *>(&sin6->sin6_addr), sizeof(sin6->sin6_addr)}, domain_t::kIpv6
            );
        }
        return std::unexpected(make_error_code(network_error::kInvalidDomain));
    }

    /**
     * @brief Extracts the address family from the endpoint.
     * @return domain_t；地址族不受支持时返回 network_error::kInvalidDomain。
     */
    [[nodiscard]] auto domain() const -> result<domain_t> {
        if(m_p->m_storage.ss_family == AF_INET) {
            return domain_t::kIpv4;
        }
        if(m_p->m_storage.ss_family == AF_INET6) {
            return domain_t::kIpv6;
        }
        return std::unexpected(make_error_code(network_error::kInvalidDomain));
    }

    /**
     * @brief Extracts the the port from the endpoint.
     * @return 主机字节序端口号；地址族不受支持时返回 network_error::kInvalidDomain。
     */
    [[nodiscard]] auto port() const -> result<std::uint16_t> {
        if(m_p->m_storage.ss_family == AF_INET) {
            return ntohs(reinterpret_cast<const sockaddr_in *>(&m_p->m_storage)->sin_port);
        }
        if(m_p->m_storage.ss_family == AF_INET6) {
            return ntohs(reinterpret_cast<const sockaddr_in6 *>(&m_p->m_storage)->sin6_port);
        }
        return std::unexpected(make_error_code(network_error::kInvalidDomain));
    }

    /// 相等比较。任一端地址族非法时视为不相等（运算符无法返回 expected）。
    auto operator==(const socket_address &other) const -> bool {
        if(m_p->m_len != other.m_p->m_len) { return false; }
        auto d = domain(), od = other.domain();
        if(!d || !od || *d != *od) { return false; }
        auto p = port(), op = other.port();
        if(!p || !op || *p != *op) { return false; }
        auto a = ip(), oa = other.ip();
        return a && oa && *a == *oa;
    }

    /**
     * @brief Creates an empty endpoint for late initialisation.
     */
    static auto make_uninitialised() -> socket_address { return socket_address{}; }

    /// 转为 "ip:port" 文本。地址族非法或 ip 转换失败时返回对应错误码。
    auto to_string() const -> result<std::string> {
        auto addr = ip();
        if(!addr) { return std::unexpected(addr.error()); }
        auto text = addr->to_string();
        if(!text) { return std::unexpected(text.error()); }
        auto p = port();
        if(!p) { return std::unexpected(p.error()); }
        return *text + ":" + std::to_string(*p);
    }

  private:
    // It's private to avoid default empty initialisation and to make use more explicit make_uninitialised
    socket_address() = default;
};

/// 流输出。地址族非法时输出错误描述而非抛异常。
inline auto operator<<(std::ostream &os, const socket_address &ep) -> std::ostream & {
    auto text = ep.to_string();
    return os << (text ? *text : std::string{"<invalid socket_address: "} + text.error().message() + ">");
}

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

    /// 映射为操作系统 socket 类型常量；枚举非法时返回
    /// network_error::kInvalidSocketType。
    static auto type_to_os(type_t type) -> result<int>;

    socket() = default;
    explicit socket(int fd): m_fd(fd) {}

#    ifdef _WIN32
    // Windows has no dup() for SOCKET handles; shallow-copy the handle.
    socket(const socket &other): m_fd(other.m_fd) {}
#    else
    socket(const socket &other): m_fd(dup(other.m_fd)) {}
#    endif
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
auto make_socket(const socket::options &opts, domain_t) -> result<socket>;

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
auto make_accept_socket(const socket::options &opts, const network::socket_address &endpoint,
                        int32_t backlog) -> result<socket>;

} // namespace silicon::network
