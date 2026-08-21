// Implementation unit for silicon::network::udp::peer.
//
// PIMPL：struct peer::impl 在此定义；所有触及实现细节的方法体（含模板 sendto/recvfrom
// 与协程 task<> 方法）也集中于此，以确保接口单元不暴露实现类型。

module;

#include <cerrno>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <expected>
#include <memory>
#include <span>
#include <system_error>
#include <tuple>

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <sys/socket.h>
#endif

module silicon.network;

import silicon.coroutine;
import silicon.scheduler;

namespace silicon::network::udp {

struct peer::impl {
    /// The scheduler that will drive this udp client.
    silicon::scheduler::io_scheduler *m_scheduler{nullptr};
    /// The udp socket.
    network::socket m_socket{-1};
    /// Did the user request this udp socket is bound locally to receive packets?
    bool m_bound{false};

    /**
     * Readiness flags for epoll Edge-Triggered (ET) mode.
     * In ET mode, notifications are only sent when the descriptor state changes.
     * These flags cache the readiness state to avoid unnecessary poll() calls.
     */

    /// True if the socket might have data to read.
    /// Must be set to true after polling.
    /// Must be set to false after recv() returns EAGAIN/EWOULDBLOCK.
    /// false by default, because the socket is usually not ready for reading on creation
    bool m_is_read_ready{false};

    /// True if the socket send buffer can accept data.
    /// Must be set to true after polling.
    /// Must be set to false after send() returns EAGAIN/EWOULDBLOCK.
    /// true by default, because the socket is usually already ready for writing on creation
    bool m_is_write_ready{true};
};

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
    : impl_(std::make_unique<impl>()) {
    impl_->m_scheduler = scheduler;
    impl_->m_socket = std::move(sock);
    impl_->m_bound = bound;
}

peer::~peer() = default;

peer::peer(peer &&other) noexcept
    : impl_(std::move(other.impl_)) {
}

peer::peer(const peer &other) noexcept
    : impl_(std::make_unique<impl>()) {
    impl_->m_scheduler = other.impl_->m_scheduler;
    impl_->m_socket = other.impl_->m_socket;
    impl_->m_bound = other.impl_->m_bound;
}

auto peer::operator=(peer &&other) noexcept -> peer & {
    if(std::addressof(other) != this) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

auto peer::operator=(const peer &other) noexcept -> peer & {
    if(std::addressof(other) != this) {
        impl_ = std::make_unique<impl>();
        impl_->m_scheduler = other.impl_->m_scheduler;
        impl_->m_socket = other.impl_->m_socket;
        impl_->m_bound = other.impl_->m_bound;
    }
    return *this;
}

auto peer::socket() noexcept -> network::socket & { return impl_->m_socket; }

auto peer::socket() const noexcept -> const network::socket & { return impl_->m_socket; }

silicon::scheduler::task<io_status> peer::write_to_impl(
        const socket_address &address,
        const std::span<const std::byte> buffer,
        std::chrono::milliseconds timeout
) {
    if(buffer.empty()) {
        co_return io_status{io_status::kind::kOk};
    }

    // Fast path
    if(impl_->m_is_write_ready) {
        auto status = sendto(address, buffer);
        if(status.try_again()) {
            // Failed to write, marking as unready and going to poll
            impl_->m_is_write_ready = false;
        } else {
            // Operation was successful (error is a success too)
            co_return status;
        }
    }

    auto pstatus = co_await poll(silicon::coroutine::poll_op::write, timeout);
    if(pstatus != silicon::coroutine::poll_status::write) {
        co_return make_io_status_from_poll_status(pstatus);
    }
    impl_->m_is_write_ready = true;

    co_return sendto(address, buffer);
}

silicon::scheduler::task<std::tuple<io_status, socket_address, std::span<std::byte>>> peer::read_from_impl(std::span<std::byte> buffer, std::chrono::milliseconds timeout) {
    // The user must bind locally to be able to receive packets.
    if(!impl_->m_bound) {
        co_return {io_status{io_status::kind::kUdpNotBound}, network::socket_address::make_uninitialised(), {}};
    }

    if(buffer.empty()) {
        co_return {io_status{io_status::kind::kOk}, network::socket_address::make_uninitialised(), {}};
    }

    // Fast path
    if(impl_->m_is_read_ready) {
        auto [status, addr, read] = recvfrom(buffer);

        if(status.try_again()) {
            // Failed to read, marking as unready and going to poll
            impl_->m_is_read_ready = false;
        } else {
            // Operation was successful (error is a success too)
            co_return {status, addr, read};
        }
    }

    auto pstatus = co_await poll(silicon::coroutine::poll_op::read, timeout);
    if(pstatus != silicon::coroutine::poll_status::read) {
        co_return {make_io_status_from_poll_status(pstatus), socket_address::make_uninitialised(), {}};
    }
    impl_->m_is_read_ready = true;

    co_return recvfrom(buffer);
}

silicon::scheduler::task<silicon::coroutine::poll_status> peer::poll(silicon::coroutine::poll_op op, std::chrono::milliseconds timeout) {
    co_return co_await impl_->m_scheduler->poll(impl_->m_socket.native_handle(), op, timeout);
}

template<silicon::coroutine::concepts::const_buffer buffer_type>
auto peer::sendto(const network::socket_address &endpoint, const buffer_type &buffer) -> io_status {
    auto [sockaddr, socklen] = endpoint.data();

    auto bytes_sent = ::sendto(
            impl_->m_socket.native_handle(),
            reinterpret_cast<const char *>(buffer.data()),
            buffer.size(),
            0,
            sockaddr,
            socklen
    );

    if(bytes_sent != -1) {
        return io_status{io_status::kind::kOk};
    } else {
        return make_io_status_from_native(errno);
    }
}

template<
        silicon::coroutine::concepts::mutable_buffer buffer_type,
        typename element_type = typename silicon::coroutine::concepts::mutable_buffer_traits<buffer_type>::element_type>
std::tuple<io_status, network::socket_address, std::span<element_type>> peer::recvfrom(buffer_type &&buffer) {
    auto endpoint = network::socket_address::make_uninitialised();
    auto [sockaddr, socklen] = endpoint.native_mutable_data();

    auto bytes_read = ::recvfrom(
            impl_->m_socket.native_handle(),
            reinterpret_cast<char *>(buffer.data()),
            buffer.size(),
            0,
            sockaddr,
            socklen
    );

    if(bytes_read < 0) {
        return {
                make_io_status_from_native(errno),
                network::socket_address::make_uninitialised(),
                std::span<element_type>{}
        };
    }

    return {
            io_status{io_status::kind::kOk},
            endpoint,
            std::span<element_type>{buffer.data(), static_cast<size_t>(bytes_read)}
    };
}

} // namespace silicon::network::udp
