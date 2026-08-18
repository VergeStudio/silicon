// Implementation unit for silicon::network::tcp::client.

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

namespace silicon::network::tcp {

using namespace std::chrono_literals;

// ── pImpl: data + (manual) copy/move so readiness flags keep their defaults ──
struct client::impl {
    silicon::scheduler::io_scheduler *m_scheduler{nullptr};
    socket_address m_endpoint;
    network::socket m_socket{-1};
    std::optional<network::connect_status> m_connect_status{std::nullopt};
    bool m_is_read_ready{false};
    bool m_is_write_ready{false};

    impl() = default;

    impl(silicon::scheduler::io_scheduler *scheduler, network::socket socket, const network::socket_address &endpoint);
    impl(silicon::scheduler::io_scheduler *scheduler, network::socket_address endpoint, network::socket sock);

    impl(const impl &other);
    impl(impl &&other) noexcept;
    auto operator=(const impl &other) noexcept -> impl &;
    auto operator=(impl &&other) noexcept -> impl &;
    ~impl();
};

client::impl::impl(silicon::scheduler::io_scheduler *scheduler, network::socket socket, const network::socket_address &endpoint)
    : m_scheduler(scheduler),
      m_endpoint(std::move(endpoint)),
      m_socket(std::move(socket)),
      m_connect_status(connect_status::kConnected) {
    // scheduler is assumed good since it comes from a tcp::server.
    // Force the socket to be non-blocking.
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
    // Readiness flags intentionally left at their defaults (false / true),
    // matching the pre-pImpl copy semantics.
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

// ── private templates (recv / send) — defined before the *_impl callers ──────
template<
        silicon::coroutine::concepts::mutable_buffer buffer_type,
        typename element_type = typename silicon::coroutine::concepts::mutable_buffer_traits<buffer_type>::element_type>
auto client::recv(buffer_type &&buffer) -> std::pair<io_status, std::span<element_type>> {
    auto bytes_recv = ::recv(impl_->m_socket.native_handle(), reinterpret_cast<char *>(buffer.data()), buffer.size(), 0);
    if(bytes_recv > 0) {
        // Ok, we've received some data.
        return {
                io_status{io_status::kind::kOk},
                std::span<element_type>{buffer.data(), static_cast<size_t>(bytes_recv)}
        };
    }

    if(bytes_recv == 0) {
        // On TCP stream sockets 0 indicates the connection has been closed by the peer.
        return {io_status{io_status::kind::kClosed}, std::span<element_type>{}};
    }

    // Report the error to the user.
    return {make_io_status_from_native(errno), std::span<element_type>{}};
}

template<
        silicon::coroutine::concepts::const_buffer buffer_type,
        typename element_type = typename silicon::coroutine::concepts::const_buffer_traits<buffer_type>::element_type>
auto client::send(const buffer_type &buffer) -> std::pair<io_status, std::span<element_type>> {
    auto bytes_sent = ::send(impl_->m_socket.native_handle(), reinterpret_cast<const char *>(buffer.data()), buffer.size(), 0);
    if(bytes_sent >= 0) {
        // Some or all of the bytes were written.
        return {
                io_status{io_status::kind::kOk},
                std::span<element_type>{buffer.data() + bytes_sent, buffer.size() - bytes_sent}
        };
    }

    // Due to the error none of the bytes were written.
    return {make_io_status_from_native(errno), std::span<element_type>{buffer.data(), buffer.size()}};
}

// ── private *_impl methods (defined in the impl unit; impl is complete here) ─
auto client::read_some_impl(std::span<std::byte> buffer, const std::chrono::milliseconds timeout)
        -> silicon::scheduler::task<std::pair<io_status, std::span<std::byte>>> {
    // Fast path
    if(impl_->m_is_read_ready) {
        auto [status, read] = recv(buffer);

        if(status.try_again()) {
            // Failed to read, marking as unready and going to poll
            impl_->m_is_read_ready = false;
        } else {
            // Operation was successful (error is a success too)
            co_return {status, read};
        }
    }

    auto poll_status = co_await poll(silicon::coroutine::poll_op::read, timeout);
    if(poll_status != silicon::coroutine::poll_status::read) {
        co_return std::pair{make_io_status_from_poll_status(poll_status), std::span<std::byte>{}};
    }
    impl_->m_is_read_ready = true;

    co_return recv(buffer);
}

auto client::read_exact_impl(std::span<std::byte> buffer, const std::chrono::milliseconds timeout)
        -> silicon::scheduler::task<std::pair<io_status, std::span<std::byte>>> {
    const auto start_time = std::chrono::steady_clock::now();
    std::span<std::byte> remaining = buffer;

    while(!remaining.empty()) {
        std::chrono::milliseconds remaining_timeout{0};
        if(timeout.count() > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time
            );

            if(elapsed >= timeout) {
                // Returning read prefix of the span
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

auto client::write_some_impl(std::span<const std::byte> buffer, const std::chrono::milliseconds timeout)
        -> silicon::scheduler::task<std::pair<io_status, std::span<const std::byte>>> {
    // Fast path
    if(impl_->m_is_write_ready) {
        auto [status, unsent] = send(buffer);

        if(status.try_again()) {
            // Failed to read, marking as unready and goint to poll
            impl_->m_is_write_ready = false;
        } else {
            // Operation was successful (error is a success too)
            co_return {status, unsent};
        }
    }

    // Waiting for readiness
    auto pstatus = co_await poll(silicon::coroutine::poll_op::write, timeout);
    if(pstatus != silicon::coroutine::poll_status::write) {
        co_return std::pair{make_io_status_from_poll_status(pstatus), buffer};
    }
    impl_->m_is_write_ready = true;

    co_return send(buffer);
}

auto client::write_all_impl(std::span<const std::byte> buffer, const std::chrono::milliseconds timeout)
        -> silicon::scheduler::task<std::pair<io_status, std::span<const std::byte>>> {
    const auto start_time = std::chrono::steady_clock::now();
    std::span<const std::byte> remaining = buffer;

    while(!remaining.empty()) {
        std::chrono::milliseconds remaining_timeout{0};
        if(timeout.count() > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time
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

auto client::poll(const silicon::coroutine::poll_op op, const std::chrono::milliseconds timeout)
        -> silicon::scheduler::task<silicon::coroutine::poll_status> {
    return impl_->m_scheduler->poll(impl_->m_socket.native_handle(), op, timeout);
}

// ── public / factory surface ─────────────────────────────────────────────────
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

auto client::connect(std::chrono::milliseconds timeout) -> silicon::scheduler::task<connect_status> {
    // Only allow the user to connect per tcp client once, if they need to re-connect they should
    // make a new tcp::client.
    if(impl_->m_connect_status.has_value()) {
        co_return impl_->m_connect_status.value();
    }

    // This enforces the connection status is aways set on the client object upon returning.
    auto return_value = [this](connect_status s) -> connect_status {
        impl_->m_connect_status = s;
        return s;
    };

    auto cret = impl_->m_socket.connect(impl_->m_endpoint);
    if(cret == 0) {
        co_return return_value(connect_status::kConnected);
    } else {
        // If the connect is happening in the background poll for write on the socket to trigger
        // when the connection is established.
        if(impl_->m_socket.in_progress()) {
            auto pstatus = co_await impl_->m_scheduler->poll(impl_->m_socket.native_handle(), silicon::coroutine::poll_op::write, timeout);
            if(pstatus == silicon::coroutine::poll_status::write) {
                int result{0};
                socklen_t result_length{sizeof(result)};
                if(::getsockopt(impl_->m_socket.native_handle(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&result), &result_length) < 0) {
                    std::cerr << "connect failed to getsockopt after write poll event\n";
                }

                if(result == 0) {
                    co_return return_value(connect_status::kConnected);
                }
            } else if(pstatus == silicon::coroutine::poll_status::timeout) {
                co_return return_value(connect_status::kTimeout);
            }
        }
    }

    co_return return_value(connect_status::kError);
}

} // namespace silicon::network::tcp
