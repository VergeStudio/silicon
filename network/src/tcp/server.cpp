#include <memory>

#include "silicon/network_impl_includes.hpp"


namespace silicon::network::tcp {
server::server(std::unique_ptr<silicon::coroutine::IScheduler> &scheduler, const network::socket_address &endpoint, options opts)
    : m_scheduler(scheduler.get()),
      m_options(std::move(opts)),
      m_accept_socket(
              network::make_accept_socket(
                      network::socket::options{socket::type_t::tcp, network::socket::blocking_t::no},
                      endpoint,
                      m_options.backlog
              )
      ) {
    if(m_scheduler == nullptr) {
        throw std::runtime_error{"tcp::server cannot have a nullptr scheduler"};
    }
}

server::server(server &&other)
    : m_scheduler(std::exchange(other.m_scheduler, nullptr)),
      m_options(std::move(other.m_options)),
      m_accept_socket(std::move(other.m_accept_socket)) {
}

auto server::operator=(server &&other) -> server & {
    if(std::addressof(other) != this) {
        m_scheduler = std::exchange(other.m_scheduler, nullptr);
        m_options = std::move(other.m_options);
        m_accept_socket = std::move(other.m_accept_socket);
    }
    return *this;
}

} // namespace silicon::network::tcp
