// Implementation unit for silicon::network::tcp::server.

module;

#include <expected>
#include <memory>
#include <system_error>

module silicon.network;

import silicon.scheduler;

namespace silicon::network::tcp {
auto server::create(std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler, const network::socket_address &endpoint, options opts)
        -> network::result<server> {
    if(scheduler == nullptr) {
        return std::unexpected(make_error_code(network_error::kNullScheduler));
    }

    auto accept_socket = network::make_accept_socket(
            network::socket::options{socket::type_t::tcp, network::socket::blocking_t::no}, endpoint, opts.backlog
    );
    if(!accept_socket) {
        return std::unexpected(accept_socket.error());
    }

    return server{scheduler.get(), std::move(opts), std::move(*accept_socket)};
}

server::server(silicon::scheduler::io_scheduler *scheduler, options opts, network::socket accept_socket)
    : m_scheduler(scheduler),
      m_options(std::move(opts)),
      m_accept_socket(std::move(accept_socket)) {
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
