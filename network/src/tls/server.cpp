// Implementation unit for silicon::network::tls::server.

module;

#ifdef SILICON_FEATURE_TLS
#    include <memory>
#    include <chrono>
#endif

module silicon.network;

#ifdef SILICON_FEATURE_TLS

import silicon.scheduler;
import silicon.scheduler.task;

namespace silicon::network::tls {
server::server(
        std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler,
        std::shared_ptr<context> tls_ctx,
        const network::socket_address &endpoint,
        options opts
)
    : m_scheduler(scheduler.get()),
      m_tls_ctx(std::move(tls_ctx)),
      m_options(std::move(opts)),
      m_accept_socket(
              network::make_accept_socket(
                      network::socket::options{network::socket::type_t::tcp, network::socket::blocking_t::no},
                      endpoint,
                      m_options.backlog
              )
      ) {
    if(m_scheduler == nullptr) {
        throw std::runtime_error{"tls::server cannot have a nullptr scheduler"};
    }

    if(m_tls_ctx == nullptr) {
        throw std::runtime_error{"tls::server cannot have a nullptr tls_ctx"};
    }
}

server::server(server &&other)
    : m_scheduler(std::exchange(other.m_scheduler, nullptr)),
      m_tls_ctx(std::move(other.m_tls_ctx)),
      m_options(std::move(other.m_options)),
      m_accept_socket(std::move(other.m_accept_socket)) {
}

auto server::operator=(server &&other) -> server & {
    if(std::addressof(other) != this) {
        m_scheduler = std::exchange(other.m_scheduler, nullptr);
        m_tls_ctx = std::move(other.m_tls_ctx);
        m_options = std::move(other.m_options);
        m_accept_socket = std::move(other.m_accept_socket);
    }
    return *this;
}

auto server::accept(std::chrono::milliseconds timeout) -> silicon::scheduler::task<silicon::network::tls::client> {
    auto client_endpoint = network::socket_address::make_uninitialised();

    network::socket s = m_accept_socket.accept(client_endpoint);

    auto tls_client = tls::client{m_scheduler, m_tls_ctx, std::move(s), client_endpoint};

    auto hstatus = co_await tls_client.handshake(timeout);
    (void)hstatus; // user must check result.
    co_return std::move(tls_client);
};

} // namespace silicon::network::tls

#endif // #ifdef SILICON_FEATURE_TLS
