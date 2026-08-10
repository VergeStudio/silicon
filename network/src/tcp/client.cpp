// Implementation unit for silicon::network::tcp::client.

module;

#if defined(_WIN32) || defined(_WIN64)
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <sys/socket.h>
#    include <unistd.h>
#endif

#include <chrono>
#include <iostream>
#include <memory>

module silicon.network;

import silicon.coroutine;
import silicon.scheduler;
import silicon.scheduler.task;

namespace silicon::network::tcp {
using namespace std::chrono_literals;

client::client(std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler, network::socket_address endpoint)
    : m_scheduler(scheduler.get()),
      m_endpoint(std::move(endpoint)),
      m_socket(
              network::make_socket(network::socket::options{socket::type_t::tcp, network::socket::blocking_t::no}, endpoint.domain())
      ) {
    if(m_scheduler == nullptr) {
        throw std::runtime_error{"tcp::client cannot have nullptr scheduler"};
    }
}

client::client(silicon::scheduler::io_scheduler *scheduler, network::socket socket, const network::socket_address &endpoint)
    : m_scheduler(scheduler),
      m_endpoint(std::move(endpoint)),
      m_socket(std::move(socket)),
      m_connect_status(connect_status::kConnected) {
    // scheduler is assumed good since it comes from a tcp::server.

    // Force the socket to be non-blocking.
    m_socket.blocking(silicon::network::socket::blocking_t::no);
}

client::client(const client &other)
    : m_scheduler(other.m_scheduler),
      m_endpoint(other.m_endpoint),
      m_socket(other.m_socket),
      m_connect_status(other.m_connect_status) {
}

client::client(client &&other) noexcept
    : m_scheduler(other.m_scheduler),
      m_endpoint(std::move(other.m_endpoint)),
      m_socket(std::move(other.m_socket)),
      m_connect_status(std::exchange(other.m_connect_status, std::nullopt)) {
}

client::~client() {
}

auto client::operator=(const client &other) noexcept -> client & {
    if(std::addressof(other) != this) {
        m_scheduler = other.m_scheduler;
        m_endpoint = other.m_endpoint;
        m_socket = other.m_socket;
        m_connect_status = other.m_connect_status;
    }
    return *this;
}

auto client::operator=(client &&other) noexcept -> client & {
    if(std::addressof(other) != this) {
        m_scheduler = std::exchange(other.m_scheduler, nullptr);
        m_endpoint = std::move(other.m_endpoint);
        m_socket = std::move(other.m_socket);
        m_connect_status = std::exchange(other.m_connect_status, std::nullopt);
    }
    return *this;
}

auto client::connect(std::chrono::milliseconds timeout) -> silicon::scheduler::task<connect_status> {
    // Only allow the user to connect per tcp client once, if they need to re-connect they should
    // make a new tcp::client.
    if(m_connect_status.has_value()) {
        co_return m_connect_status.value();
    }

    // This enforces the connection status is aways set on the client object upon returning.
    auto return_value = [this](connect_status s) -> connect_status {
        m_connect_status = s;
        return s;
    };

    auto cret = m_socket.connect(m_endpoint);
    if(cret == 0) {
        co_return return_value(connect_status::kConnected);
    } else {
        // If the connect is happening in the background poll for write on the socket to trigger
        // when the connection is established.
        if(m_socket.in_progress()) {
            auto pstatus = co_await m_scheduler->poll(m_socket.native_handle(), silicon::coroutine::poll_op::write, timeout);
            if(pstatus == silicon::coroutine::poll_status::write) {
                int result{0};
                socklen_t result_length{sizeof(result)};
                if(::getsockopt(m_socket.native_handle(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&result), &result_length) < 0) {
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
