module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <sys/socket.h>
#    include <unistd.h>
#endif

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <expected>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <system_error>
#include <coroutine>

module silicon.network;

import silicon.coroutine;
import silicon.scheduler;
import silicon.scheduler.task;
import silicon.time;

namespace silicon::network::tcp {

using namespace std::chrono_literals;

struct client::impl {
    silicon::scheduler::io_scheduler *m_scheduler{nullptr};
    socket_address m_endpoint;
    network::socket m_socket{-1};
    std::optional<network::connect_status> m_connect_status{std::nullopt};
    bool m_is_read_ready{false};
    bool m_is_write_ready{false};

    impl() = delete;

    impl(silicon::scheduler::io_scheduler *scheduler, network::socket socket, const network::socket_address &endpoint);
    impl(silicon::scheduler::io_scheduler *scheduler, network::socket_address endpoint, network::socket sock);

    impl(const impl &other);
    impl(impl &&other) noexcept;
    impl & operator=(const impl &other) noexcept ;
    impl & operator=(impl &&other) noexcept ;
    ~impl();
};

client::impl::impl(silicon::scheduler::io_scheduler *scheduler, network::socket socket, const network::socket_address &endpoint)
    : m_scheduler(scheduler),
      m_endpoint(std::move(endpoint)),
      m_socket(std::move(socket)),
      m_connect_status(connect_status::kConnected) {

    m_socket.blocking(silicon::network::socket::blocking_t::no);
}

client::impl::impl(silicon::scheduler::io_scheduler *scheduler, network::socket_address endpoint, network::socket sock)
    : m_scheduler(scheduler),
      m_endpoint(std::move(endpoint)),
      m_socket(std::move(sock)) {
}

client::impl::impl(const impl &other)
    : m_scheduler(other.m_scheduler),
      m_endpoint(other.m_endpoint),
      m_socket(other.m_socket),
      m_connect_status(other.m_connect_status) {

}

client::impl::impl(impl &&other) noexcept
    : m_scheduler(other.m_scheduler),
      m_endpoint(std::move(other.m_endpoint)),
      m_socket(std::move(other.m_socket)),
      m_connect_status(std::exchange(other.m_connect_status, std::nullopt)) {
}

auto client::impl::operator=(const impl &other) noexcept -> impl & {
    if(std::addressof(other) != this) {
        m_scheduler = other.m_scheduler;
        m_endpoint = other.m_endpoint;
        m_socket = other.m_socket;
        m_connect_status = other.m_connect_status;
    }
    return *this;
}

auto client::impl::operator=(impl &&other) noexcept -> impl & {
    if(std::addressof(other) != this) {
        m_scheduler = std::exchange(other.m_scheduler, nullptr);
        m_endpoint = std::move(other.m_endpoint);
        m_socket = std::move(other.m_socket);
        m_connect_status = std::exchange(other.m_connect_status, std::nullopt);
    }
    return *this;
}

client::impl::~impl() = default;

template<
        silicon::scheduler::concepts::mutable_buffer buffer_type,
        typename element_type>
std::pair<io_status, std::span<element_type>> client::recv(buffer_type &&buffer) {
    auto bytes_recv = ::recv(impl_->m_socket.native_handle(), reinterpret_cast<char *>(buffer.data()), buffer.size(), 0);
    if(bytes_recv > 0) {

        return {
                io_status{io_status::kind::kOk},
                std::span<element_type>{buffer.data(), static_cast<size_t>(bytes_recv)}
        };
    }

    if(bytes_recv == 0) {

        return {io_status{io_status::kind::kClosed}, std::span<element_type>{}};
    }

    return {make_io_status_from_native(errno), std::span<element_type>{}};
}

template<
        silicon::scheduler::concepts::const_buffer buffer_type,
        typename element_type>
std::pair<io_status, std::span<element_type>> client::send(const buffer_type &buffer) {
    auto bytes_sent = ::send(impl_->m_socket.native_handle(), reinterpret_cast<const char *>(buffer.data()), buffer.size(), 0);
    if(bytes_sent >= 0) {

        return {
                io_status{io_status::kind::kOk},
                std::span<element_type>{buffer.data() + bytes_sent, buffer.size() - bytes_sent}
        };
    }

    return {make_io_status_from_native(errno), std::span<element_type>{buffer.data(), buffer.size()}};
}

silicon::scheduler::task<std::pair<io_status, std::span<std::byte>>> client::read_some_impl(std::span<std::byte> buffer, const std::chrono::milliseconds timeout) {

    if(impl_->m_is_read_ready) {
        auto [status, read] = recv(buffer);

        if(status.try_again()) {

            impl_->m_is_read_ready = false;
        } else {

            co_return {status, read};
        }
    }

    auto poll_status = co_await poll(silicon::scheduler::poll_op::read, timeout);
    if(poll_status != silicon::scheduler::poll_status::read) {
        co_return std::pair{make_io_status_from_poll_status(poll_status), std::span<std::byte>{}};
    }
    impl_->m_is_read_ready = true;

    co_return recv(buffer);
}

silicon::scheduler::task<std::pair<io_status, std::span<std::byte>>> client::read_exact_impl(std::span<std::byte> buffer, const std::chrono::milliseconds timeout) {
    const auto start_time = silicon::time::steady_clock::now();
    std::span<std::byte> remaining = buffer;

    while(!remaining.empty()) {
        std::chrono::milliseconds remaining_timeout{0};
        if(timeout.count() > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    silicon::time::steady_clock::now() - start_time
            );

            if(elapsed >= timeout) {

                co_return {
                        io_status{io_status::kind::kTimeout}, buffer.subspan(0, buffer.size() - remaining.size())
                };
            }
            remaining_timeout = timeout - elapsed;
        }

        auto [status, read_span] = co_await read_some_impl(remaining, remaining_timeout);
        remaining = remaining.subspan(read_span.size());

        if(!status.is_ok()) {
            co_return {status, buffer.subspan(0, buffer.size() - remaining.size())};
        }
    }

    co_return {io_status{io_status::kind::kOk}, buffer};
}

silicon::scheduler::task<std::pair<io_status, std::span<const std::byte>>> client::write_some_impl(std::span<const std::byte> buffer, const std::chrono::milliseconds timeout) {

    if(impl_->m_is_write_ready) {
        auto [status, unsent] = send(buffer);

        if(status.try_again()) {

            impl_->m_is_write_ready = false;
        } else {

            co_return {status, unsent};
        }
    }

    auto pstatus = co_await poll(silicon::scheduler::poll_op::write, timeout);
    if(pstatus != silicon::scheduler::poll_status::write) {
        co_return std::pair{make_io_status_from_poll_status(pstatus), buffer};
    }
    impl_->m_is_write_ready = true;

    co_return send(buffer);
}

silicon::scheduler::task<std::pair<io_status, std::span<const std::byte>>> client::write_all_impl(std::span<const std::byte> buffer, const std::chrono::milliseconds timeout) {
    const auto start_time = silicon::time::steady_clock::now();
    std::span<const std::byte> remaining = buffer;

    while(!remaining.empty()) {
        std::chrono::milliseconds remaining_timeout{0};
        if(timeout.count() > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    silicon::time::steady_clock::now() - start_time
            );

            if(elapsed >= timeout) {
                co_return {io_status{io_status::kind::kTimeout}, remaining};
            }
            remaining_timeout = timeout - elapsed;
        }

        auto [status, unsent_span] = co_await write_some_impl(remaining, remaining_timeout);
        remaining = unsent_span;

        if(!status.is_ok()) {
            co_return {status, remaining};
        }
    }

    co_return {io_status{io_status::kind::kOk}, {}};
}

silicon::scheduler::task<silicon::scheduler::poll_status> client::poll(const silicon::scheduler::poll_op op, const std::chrono::milliseconds timeout) {
    return impl_->m_scheduler->poll(impl_->m_socket.native_handle(), op, timeout);
}

auto client::create(std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler, network::socket_address endpoint)
        -> network::result<client> {
    if(scheduler == nullptr) {
        return std::unexpected(make_error_code(network_error::kNullScheduler));
    }

    auto domain = endpoint.domain();
    if(!domain) {
        return std::unexpected(domain.error());
    }

    auto sock = network::make_socket(
            network::socket::options{socket::type_t::tcp, network::socket::blocking_t::no}, *domain
    );
    if(!sock) {
        return std::unexpected(sock.error());
    }

    return client{scheduler.get(), std::move(endpoint), std::move(*sock)};
}

client::client(silicon::scheduler::io_scheduler *scheduler, network::socket socket, const network::socket_address &endpoint)
    : impl_(std::make_unique<impl>(scheduler, std::move(socket), endpoint)) {
}

client::client(silicon::scheduler::io_scheduler *scheduler, network::socket_address endpoint, network::socket sock)
    : impl_(std::make_unique<impl>(scheduler, std::move(endpoint), std::move(sock))) {
}

client::client(const client &other)
    : impl_(other.impl_ ? std::make_unique<impl>(*other.impl_) : nullptr) {
}

client::client(client &&other) noexcept
    : impl_(std::move(other.impl_)) {
}

client::~client() = default;

auto client::operator=(const client &other) noexcept -> client & {
    if(std::addressof(other) != this) {
        impl_ = other.impl_ ? std::make_unique<impl>(*other.impl_) : nullptr;
    }
    return *this;
}

auto client::operator=(client &&other) noexcept -> client & {
    if(std::addressof(other) != this) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

auto client::socket() -> network::socket & {
    return impl_->m_socket;
}

auto client::socket() const -> const network::socket & {
    return impl_->m_socket;
}

silicon::scheduler::task<connect_status> client::connect(std::chrono::milliseconds timeout) {

    if(impl_->m_connect_status.has_value()) {
        co_return impl_->m_connect_status.value();
    }

    auto return_value = [this](connect_status s) -> connect_status {
        impl_->m_connect_status = s;
        return s;
    };

    auto cret = impl_->m_socket.connect(impl_->m_endpoint);
    if(cret == 0) {
        co_return return_value(connect_status::kConnected);
    } else {

        if(impl_->m_socket.in_progress()) {
            auto pstatus = co_await impl_->m_scheduler->poll(impl_->m_socket.native_handle(), silicon::scheduler::poll_op::write, timeout);
            if(pstatus == silicon::scheduler::poll_status::write) {
                int result{0};
                socklen_t result_length{sizeof(result)};
                if(::getsockopt(impl_->m_socket.native_handle(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&result), &result_length) < 0) {
                    std::cerr << "connect failed to getsockopt after write poll event\n";
                }

                if(result == 0) {
                    co_return return_value(connect_status::kConnected);
                }
            } else if(pstatus == silicon::scheduler::poll_status::timeout) {
                co_return return_value(connect_status::kTimeout);
            }
        }
    }

    co_return return_value(connect_status::kError);
}

}
