#pragma once

#include "silicon/network/socket.hpp"

namespace silicon::network::udp {

/// @brief Abstract interface for a UDP peer.
class IUdpPeer {
  public:
    IUdpPeer() = default;
    IUdpPeer(const IUdpPeer &) = delete;
    IUdpPeer(IUdpPeer &&) = delete;
    auto operator=(const IUdpPeer &) -> IUdpPeer & = delete;
    auto operator=(IUdpPeer &&) -> IUdpPeer & = delete;
    virtual ~IUdpPeer() = default;

    virtual auto socket() noexcept -> network::socket & = 0;
    virtual auto socket() const noexcept -> const network::socket & = 0;
};

} // namespace silicon::network::udp
