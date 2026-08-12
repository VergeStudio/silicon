// Implementation unit for silicon::network::udp::peer.

module;

#include <expected>
#include <memory>
#include <system_error>

module silicon.network;

import silicon.scheduler;

namespace silicon::network::udp {
auto peer::create(std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler, network::domain_t domain)
        -> network::result<peer> {
    if(scheduler == nullptr) {
        return std::unexpected(make_error_code(network_error::kNullScheduler));
    }

    auto sock = network::make_socket(
            network::socket::options{network::socket::type_t::udp, network::socket::blocking_t::no}, domain
    );
    if(!sock) {
        return std::unexpected(sock.error());
    }

    return peer{scheduler.get(), std::move(*sock), false};
}

auto peer::create(std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler, const network::socket_address &endpoint)
        -> network::result<peer> {
    if(scheduler == nullptr) {
        return std::unexpected(make_error_code(network_error::kNullScheduler));
    }

    auto sock = network::make_accept_socket(
            network::socket::options{network::socket::type_t::udp, network::socket::blocking_t::no}, endpoint, 32
    );
    if(!sock) {
        return std::unexpected(sock.error());
    }

    return peer{scheduler.get(), std::move(*sock), true};
}

peer::peer(silicon::scheduler::io_scheduler *scheduler, network::socket sock, bool bound)
    : m_scheduler(scheduler),
      m_socket(std::move(sock)),
      m_bound(bound) {
}

peer::peer(peer &&other) noexcept
    : m_scheduler(std::exchange(other.m_scheduler, nullptr)),
      m_socket(std::move(other.m_socket)),
      m_bound(other.m_bound) {
}

peer::peer(const peer &other) noexcept
    : m_scheduler(other.m_scheduler),
      m_socket(other.m_socket),
      m_bound(other.m_bound) {
}

auto peer::operator=(peer &&other) noexcept -> peer & {
    if(std::addressof(other) != this) {
        m_scheduler = std::exchange(other.m_scheduler, nullptr);
        m_socket = std::move(other.m_socket);
        m_bound = other.m_bound;
    }
    return *this;
}

auto peer::operator=(const peer &other) noexcept -> peer & {
    if(std::addressof(other) != this) {
        m_scheduler = other.m_scheduler;
        m_socket = other.m_socket;
        m_bound = other.m_bound;
    }
    return *this;
}
} // namespace silicon::network::udp
