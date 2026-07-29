#include <memory>

#include "silicon/network_impl_includes.hpp"


namespace silicon::network::udp {
peer::peer(std::unique_ptr<silicon::coroutine::IScheduler> &scheduler, network::domain_t domain)
    : m_scheduler(scheduler.get()),
      m_socket(network::make_socket(network::socket::options{network::socket::type_t::udp, network::socket::blocking_t::no}, domain)) {
    if(m_scheduler == nullptr) {
        throw std::runtime_error("udp::peer cannot have nullptr scheduler");
    }
}

peer::peer(std::unique_ptr<silicon::coroutine::IScheduler> &scheduler, const network::socket_address &endpoint)
    : m_scheduler(scheduler.get()),
      m_socket(
              network::make_accept_socket(
                      network::socket::options{network::socket::type_t::udp, network::socket::blocking_t::no},
                      endpoint,
                      32
              )
      ),
      m_bound(true) {
    if(m_scheduler == nullptr) {
        throw std::runtime_error("udp::peer cannot have nullptr scheduler");
    }
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
