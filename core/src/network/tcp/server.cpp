module;

#include <chrono>
#include <expected>
#include <memory>
#include <system_error>
#include <coroutine>

module silicon.network;
import silicon.error;

import silicon.coroutine;
import silicon.scheduler;

namespace silicon::network::tcp {

struct server::impl {
    silicon::scheduler::io_scheduler *m_scheduler{nullptr};
    options m_options;
    network::socket m_accept_socket{-1};
    silicon::scheduler::poll_stop_source m_cancel_trigger{};
    bool m_is_read_ready{false};

    impl() = default;

    impl(silicon::scheduler::io_scheduler *scheduler, options opts, network::socket accept_socket);
    impl(impl &&other) noexcept;
    impl & operator=(impl &&other) noexcept ;
    ~impl();
};

server::impl::impl(silicon::scheduler::io_scheduler *scheduler, options opts, network::socket accept_socket)
    : m_scheduler(scheduler),
      m_options(std::move(opts)),
      m_accept_socket(std::move(accept_socket)) {

}

server::impl::impl(impl &&other) noexcept
    : m_scheduler(std::exchange(other.m_scheduler, nullptr)),
      m_options(std::move(other.m_options)),
      m_accept_socket(std::move(other.m_accept_socket)) {
}

auto server::impl::operator=(impl &&other) noexcept -> impl & {
    if(std::addressof(other) != this) {
        m_scheduler = std::exchange(other.m_scheduler, nullptr);
        m_options = std::move(other.m_options);
        m_accept_socket = std::move(other.m_accept_socket);
    }
    return *this;
}

server::impl::~impl() = default;

auto server::create(std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler, const network::socket_address &endpoint, options opts)
        -> silicon::error::result<server> {
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
    : impl_(std::make_unique<impl>(scheduler, std::move(opts), std::move(accept_socket))) {
}

server::server(server &&other)
    : impl_(std::move(other.impl_)) {
}

auto server::operator=(server &&other) -> server & {
    if(std::addressof(other) != this) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

server::~server() = default;

auto server::accept_socket() -> network::socket & {
    return impl_->m_accept_socket;
}

auto server::accept_socket() const -> const network::socket & {
    return impl_->m_accept_socket;
}

auto server::shutdown() {
    impl_->m_cancel_trigger.signal_stop();
    impl_->m_accept_socket.shutdown(silicon::scheduler::poll_op::read_write);
}

silicon::scheduler::task<std::expected<network::tcp::client, io_status>> server::accept(std::chrono::milliseconds timeout) {

    if(impl_->m_is_read_ready) {
        auto client = accept_now();
        if(!client && client.error().try_again()) {

            impl_->m_is_read_ready = false;
        } else {

            co_return client;
        }
    }

    auto pstatus = co_await poll(timeout);
    if(pstatus != silicon::scheduler::poll_status::read) {
        co_return std::unexpected<io_status>{make_io_status_from_poll_status(pstatus)};
    }
    impl_->m_is_read_ready = true;

    co_return accept_now();
}

silicon::scheduler::task<silicon::scheduler::poll_status> server::poll(std::chrono::milliseconds timeout) {
    return impl_->m_scheduler->poll(
            impl_->m_accept_socket.native_handle(),
            silicon::scheduler::poll_op::read,
            timeout,
            impl_->m_cancel_trigger.get_token()
    );
}

std::expected<silicon::network::tcp::client, io_status> server::accept_now() {
    auto client_endpoint = socket_address::make_uninitialised();

    network::socket accepted = impl_->m_accept_socket.accept(client_endpoint);
    if(!accepted.is_ok()) {
        return std::unexpected<io_status>{make_io_status_from_native(impl_->m_accept_socket.last_error())};
    }

    return tcp::client{impl_->m_scheduler, std::move(accepted), client_endpoint};
}

}
