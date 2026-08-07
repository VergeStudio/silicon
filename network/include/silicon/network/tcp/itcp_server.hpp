#pragma once

#include <chrono>
#include <memory>

import silicon.coroutine;
#include "silicon/network/io_status.hpp"
#include "silicon/network/tcp/client.hpp"

namespace silicon::network::tcp {

/// @brief Abstract interface for a TCP server.
class ITcpServer {
  public:
    ITcpServer() = default;
    ITcpServer(const ITcpServer &) = delete;
    ITcpServer(ITcpServer &&) = delete;
    auto operator=(const ITcpServer &) -> ITcpServer & = delete;
    auto operator=(ITcpServer &&) -> ITcpServer & = delete;
    virtual ~ITcpServer() = default;

    virtual auto accept(std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task::task<silicon::coroutine::expected<client, io_status>> = 0;
};

} // namespace silicon::network::tcp
