#ifdef SILICON_FEATURE_TLS

#    pragma once

#    include <chrono>
#    include <memory>

import silicon.coroutine;

namespace silicon::network::tls {

class ITlsServer {
  public:
    ITlsServer() = default;
    ITlsServer(const ITlsServer &) = delete;
    ITlsServer(ITlsServer &&) = delete;
    auto operator=(const ITlsServer &) -> ITlsServer & = delete;
    auto operator=(ITlsServer &&) -> ITlsServer & = delete;
    virtual ~ITlsServer() = default;

    virtual auto poll(std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task::task<silicon::coroutine::poll_status> = 0;

    virtual auto accept(std::chrono::milliseconds timeout = std::chrono::seconds{30})
            -> silicon::scheduler::task::task<client> = 0;
};

} // namespace silicon::network::tls

#endif
