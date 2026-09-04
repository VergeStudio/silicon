module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <sys/socket.h>
#endif

#include <expected>
#include <memory>
#include <system_error>
#include <utility>

module silicon.network;

#if defined(_MSC_VER)
import silicon.network;
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

auto socket::operator=(socket &&other) noexcept -> socket & {
    if(std::addressof(other) != this) {
        m_fd = std::exchange(other.m_fd, -1);
    }

    return *this;
}

auto make_socket(const socket::options &opts, domain_t domain) -> result<socket> {
    auto os_type = socket::type_to_os(opts.type);
    if(!os_type) { return std::unexpected(os_type.error()); }

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

    if(socket_enable_address_reuse(s.native_handle()) == false) {
        return std::unexpected(make_error_code(network_error::kSetSockOptFailed));
    }

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

}
