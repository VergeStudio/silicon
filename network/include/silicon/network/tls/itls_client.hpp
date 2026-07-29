#ifdef SILICON_FEATURE_TLS

#    pragma once

#    include <chrono>
#    include <memory>

import silicon.coroutine;
#    include "silicon/network/tls/connection_status.hpp"

namespace silicon::network::tls {

/// @brief Abstract interface for a TLS client connection.
class ITlsClient {
  public:
    ITlsClient() = default;
    ITlsClient(const ITlsClient &) = delete;
    ITlsClient(ITlsClient &&) = delete;
    auto operator=(const ITlsClient &) -> ITlsClient & = delete;
    auto operator=(ITlsClient &&) -> ITlsClient & = delete;
    virtual ~ITlsClient() = default;

    virtual auto connect(std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::coroutine::task<connection_status> = 0;
};

} // namespace silicon::network::tls

#endif
