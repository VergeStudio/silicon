module;

#if defined(SILICON_PLATFORM_WINDOWS)
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

#include <tuple>
#include <silicon/proxy/proxy_macros.h>

#include <silicon/common.h>
export module silicon.network:facade;
export import silicon.network.error;

export import silicon.coroutine;
import silicon.proxy;
import silicon.error;

// 非导出：平台接缝——BSD socket 地址的 sin_len/sin6_len 填充。
// BSD/Apple 为真实填充，Linux/Windows 为空实现。
// 实现位于 socket_address_bsd.cpp / socket_address_linux.cpp / socket_address_win.cpp。
namespace silicon::network {

void network_set_sockaddr_len(sockaddr_in *sin);
void network_set_sockaddr_len6(sockaddr_in6 *sin6);

}

export namespace silicon::network {

[[nodiscard]] inline std::error_code system_error(int errno_value) noexcept {
    return {errno_value, std::generic_category()};
}

[[nodiscard]] inline std::error_code system_error(std::errc e) noexcept {
    return std::make_error_code(e);
}

enum class connect_status {

    kConnected,

    kInvalidIpAddress,

    kTimeout,

    kError
};

SILICON_CORE_API auto to_string(const connect_status &) -> silicon::error::result<std::string_view>;

class SILICON_CORE_API hostname {
  private:
    struct impl {
      public:
        std::string m_hostname;
    };
    std::shared_ptr<impl> m_p{std::make_shared<impl>()};

  public:
    hostname() = default;
    explicit hostname(std::string hn): m_p(std::make_shared<impl>()) { m_p->m_hostname = std::move(hn); }

    hostname(const hostname &o): m_p(std::make_shared<impl>(*o.m_p)) {}
    hostname(hostname &&) noexcept = default;
    hostname & operator=(const hostname &o) {
        if(this != &o) { m_p = std::make_shared<impl>(*o.m_p); }
        return *this;
    }
    hostname & operator=(hostname &&) noexcept = default;
    ~hostname() = default;

    auto data() const -> const std::string & { return m_p->m_hostname; }

    auto operator<=>(const hostname &other) const { return m_p->m_hostname <=> other.m_p->m_hostname; }
    bool operator==(const hostname &other) const { return m_p->m_hostname == other.m_p->m_hostname; }

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

    [[nodiscard]] bool is_ok() const { return type == kind::kOk; }
    [[nodiscard]] bool is_timeout() const { return type == kind::kTimeout; }
    [[nodiscard]] bool is_closed() const { return type == kind::kClosed; }
    [[nodiscard]] bool would_block() const { return type == kind::kWouldBlockOrTryAgain; }
    [[nodiscard]] bool try_again() const { return type == kind::kWouldBlockOrTryAgain; }

    [[nodiscard]] bool is_native() const { return type == kind::kNative; }

    explicit operator bool() const { return is_ok(); }

    [[nodiscard]] std::string message() const ;
};

std::string_view to_string(io_status::kind) ;
io_status make_io_status_from_native(int) ;
auto make_io_status_from_poll_status(silicon::scheduler::poll_status) -> io_status;

[[nodiscard]] std::string message_impl(int);
io_status make_io_status_from_native_impl(int) ;

enum class recv_status : int64_t {
    kOk = 0,

    kClosed = -1,

    kUdpNotBound = -2,
    kTryAgain = EAGAIN,

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

auto to_string(recv_status) -> const std::string &;

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

auto to_string(domain_t) -> silicon::error::result<std::string_view>;

class SILICON_CORE_API ip_address {
  public:
    static const constexpr size_t ipv4_len{4};
    static const constexpr size_t ipv6_len{16};

    ip_address() = default;

    static auto from_binary(std::span<const uint8_t> binary_address,
                            domain_t domain = domain_t::kIpv4) -> silicon::error::result<ip_address> {
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

    ip_address(const ip_address &o): m_p(std::make_shared<impl>(*o.m_p)) {}
    ip_address(ip_address &&) noexcept = default;
    ip_address & operator=(const ip_address &o) {
        if(this != &o) { m_p = std::make_shared<impl>(*o.m_p); }
        return *this;
    }
    ip_address & operator=(ip_address &&) noexcept = default;
    ~ip_address() = default;

    domain_t domain() const { return m_p->m_domain; }
    std::span<const uint8_t> data() const {
        if(m_p->m_domain == domain_t::kIpv4) {
            return std::span<const uint8_t>{m_p->m_data.data(), ipv4_len};
        } else {
            return std::span<const uint8_t>{m_p->m_data.data(), ipv6_len};
        }
    }

    static auto from_string(std::string_view address, domain_t domain = domain_t::kIpv4) -> silicon::error::result<ip_address> {
        ip_address addr{};
        addr.m_p->m_domain = domain;

        auto success = inet_pton(static_cast<int>(addr.m_p->m_domain), address.data(), addr.m_p->m_data.data());
        if(success != 1) {
            return std::unexpected(make_error_code(network_error::kInvalidIpAddress));
        }

        return addr;
    }

    auto to_string() const -> silicon::error::result<std::string> {
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
    bool operator==(const ip_address &other) const {
        return m_p->m_domain == other.m_p->m_domain && m_p->m_data == other.m_p->m_data;
    }

  private:
    struct impl {
      public:
        domain_t m_domain{domain_t::kIpv4};
        std::array<uint8_t, ipv6_len> m_data{};
    };
    std::shared_ptr<impl> m_p{std::make_shared<impl>()};
};

PRO_DEF_MEM_DISPATCH(MemSocketIsOk, is_ok);
PRO_DEF_MEM_DISPATCH(MemSocketBlocking, blocking);
PRO_DEF_MEM_DISPATCH(MemSocketShutdown, shutdown);
PRO_DEF_MEM_DISPATCH(MemSocketClose, close);
PRO_DEF_MEM_DISPATCH(MemSocketNativeHandle, native_handle);

struct socket_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemSocketIsOk, bool() const>
      ::add_convention<MemSocketBlocking, bool(int)>
      ::add_convention<MemSocketShutdown, bool(int)>
      ::add_convention<MemSocketClose, void()>
      ::add_convention<MemSocketNativeHandle, int() const>
      ::build {};

using socket_proxy = silicon::proxy::proxy<socket_facade>;

using socket_view = silicon::proxy::proxy_view<socket_facade>;

template<class T, class... Args>
[[nodiscard]] socket_proxy make_socket_proxy(Args &&...args) {
    return silicon::proxy::make_proxy<socket_facade, T>(std::forward<Args>(args)...);
}

template<class T>
    requires silicon::proxy::proxiable_target<T, socket_facade>
[[nodiscard]] socket_view make_socket_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<socket_facade>(target);
}

class SILICON_CORE_API socket_address {
  private:
    struct impl {
      public:
        sockaddr_storage m_storage{};
        socklen_t m_len = sizeof(sockaddr_storage);
    };
    std::shared_ptr<impl> m_p{std::make_shared<impl>()};

  public:

    static auto create(std::string_view ip, std::uint16_t port,
                       domain_t domain = domain_t::kIpv4) -> silicon::error::result<socket_address> {
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

            network_set_sockaddr_len(sin);

            std::memcpy(&sin->sin_addr, ip.data().data(), sizeof(in_addr));
            len = sizeof(sockaddr_in);
        } else if(ip.domain() == domain_t::kIpv6) {
            auto *sin6 = reinterpret_cast<sockaddr_in6 *>(&storage);
            sin6->sin6_family = AF_INET6;
            sin6->sin6_port = htons(port);

            network_set_sockaddr_len6(sin6);

            std::memcpy(&sin6->sin6_addr, ip.data().data(), sizeof(in6_addr));
            len = sizeof(sockaddr_in6);

        }
    }

    socket_address(const socket_address &o): m_p(std::make_shared<impl>(*o.m_p)) {}
    socket_address(socket_address &&) noexcept = default;
    socket_address & operator=(const socket_address &o) {
        if(this != &o) { m_p = std::make_shared<impl>(*o.m_p); }
        return *this;
    }
    socket_address & operator=(socket_address &&) noexcept = default;
    ~socket_address() = default;

    [[nodiscard]] std::pair<const sockaddr *, socklen_t> data() const & {
        return {reinterpret_cast<const sockaddr *>(&m_p->m_storage), m_p->m_len};
    }

    std::pair<const sockaddr *, socklen_t> data() const && = delete;

    [[nodiscard]] std::pair<sockaddr *, socklen_t *> native_mutable_data() & {
        return {reinterpret_cast<sockaddr *>(&m_p->m_storage), &m_p->m_len};
    }

    [[nodiscard]] silicon::error::result<ip_address> ip() const {
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

    [[nodiscard]] silicon::error::result<domain_t> domain() const {
        if(m_p->m_storage.ss_family == AF_INET) {
            return domain_t::kIpv4;
        }
        if(m_p->m_storage.ss_family == AF_INET6) {
            return domain_t::kIpv6;
        }
        return std::unexpected(make_error_code(network_error::kInvalidDomain));
    }

    [[nodiscard]] auto port() const -> silicon::error::result<std::uint16_t> {
        if(m_p->m_storage.ss_family == AF_INET) {
            return ntohs(reinterpret_cast<const sockaddr_in *>(&m_p->m_storage)->sin_port);
        }
        if(m_p->m_storage.ss_family == AF_INET6) {
            return ntohs(reinterpret_cast<const sockaddr_in6 *>(&m_p->m_storage)->sin6_port);
        }
        return std::unexpected(make_error_code(network_error::kInvalidDomain));
    }

    bool operator==(const socket_address &other) const {
        if(m_p->m_len != other.m_p->m_len) { return false; }
        auto d = domain(), od = other.domain();
        if(!d || !od || *d != *od) { return false; }
        auto p = port(), op = other.port();
        if(!p || !op || *p != *op) { return false; }
        auto a = ip(), oa = other.ip();
        return a && oa && *a == *oa;
    }

    static socket_address make_uninitialised() { return socket_address{}; }

    auto to_string() const -> silicon::error::result<std::string> {
        auto addr = ip();
        if(!addr) { return std::unexpected(addr.error()); }
        auto text = addr->to_string();
        if(!text) { return std::unexpected(text.error()); }
        auto p = port();
        if(!p) { return std::unexpected(p.error()); }
        return *text + ":" + std::to_string(*p);
    }

  private:

    socket_address() = default;
};

inline std::ostream & operator<<(std::ostream &os, const socket_address &ep) {
    auto text = ep.to_string();
    return os << (text ? *text : std::string{"<invalid socket_address: "} + text.error().message() + ">");
}

int socket_duplicate_handle(int) ;

bool socket_enable_address_reuse(int) ;

class SILICON_CORE_API socket final {
  public:
    enum class type_t {

        udp,

        tcp
    };

    enum class blocking_t {

        yes,

        no
    };

    struct options {

        type_t type;

        blocking_t blocking;
    };

    static silicon::error::result<int> type_to_os(type_t) ;

    socket() = default;
    explicit socket(int fd): m_fd(fd) {}

    socket(const socket &other): m_fd(socket_duplicate_handle(other.m_fd)) {}
    socket(socket &&other) noexcept: m_fd(std::exchange(other.m_fd, -1)) {}
    socket & operator=(const socket &other) noexcept ;
    socket & operator=(socket &&other) noexcept ;

    ~socket() { close(); }

    [[nodiscard]] bool is_ok() const { return m_fd != -1; }

    explicit operator bool() const { return is_ok(); }

    bool blocking(blocking_t) ;
    bool blocking(int block) { return blocking(static_cast<blocking_t>(block)); }

    bool shutdown(silicon::scheduler::poll_op = silicon::scheduler::poll_op::read_write) ;
    bool shutdown(int how) { return shutdown(static_cast<silicon::scheduler::poll_op>(how)); }

    void close() ;

    int native_handle() const { return m_fd; }

    socket accept(socket_address &) ;

    int last_error() const ;

    int connect(const socket_address &) ;

    bool in_progress() const ;

  private:
    int m_fd{-1};
};

auto make_socket(const socket::options &, domain_t) -> silicon::error::result<socket>;

auto make_accept_socket(const socket::options &, const network::socket_address &,
                        int32_t) -> silicon::error::result<socket>;

}
