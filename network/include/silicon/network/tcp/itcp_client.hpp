#pragma once

#include <chrono>
#include <memory>
#include <span>

import silicon.coroutine;
#include "silicon/network/connect.hpp"
#include "silicon/network/io_status.hpp"
#include "silicon/network/socket.hpp"
#include "silicon/network/socket_address.hpp"

namespace silicon::network::tcp {

/// @brief Abstract interface for a TCP client connection.
///
/// The concrete tcp::client class implements this interface.
/// Template methods (read_some, read_exact, write_some, write_all, recv, send)
/// remain in the concrete class — they cannot be virtual.
class ITcpClient {
  public:
    ITcpClient() = default;
    ITcpClient(const ITcpClient &) = delete;
    ITcpClient(ITcpClient &&) = delete;
    auto operator=(const ITcpClient &) -> ITcpClient & = delete;
    auto operator=(ITcpClient &&) -> ITcpClient & = delete;
    virtual ~ITcpClient() = default;

    [[nodiscard]] virtual auto socket() -> network::socket & = 0;
    [[nodiscard]] virtual auto socket() const -> const network::socket & = 0;

    virtual auto connect(std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task::task<network::connect_status> = 0;

    virtual auto poll(
            silicon::coroutine::poll_op op,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) -> silicon::scheduler::task::task<silicon::coroutine::poll_status> = 0;
};

} // namespace silicon::network::tcp
