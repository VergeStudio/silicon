#include <memory>
#ifdef SILICON_FEATURE_TLS

#    pragma once

#    if defined(_WIN32) || defined(_WIN64)
#        include <winsock2.h>
#        include <ws2tcpip.h>
#    else
#        include <fcntl.h>
#        include <sys/socket.h>
#    endif

#    include "silicon/network/ip_address.hpp"
#    include "silicon/network/socket.hpp"
#    include "silicon/network/tls/client.hpp"
#    include "silicon/network/tls/itls_server.hpp"
#include <coroutine> // task.hpp 文本包含时代经其传递获得，import 化后需显式包含
import silicon.scheduler.task; // 兼容头文本包含与 silicon.task 模块冲突，改用 import
import silicon.scheduler;

namespace silicon::network::tls {
class context;

class server final: public ITlsServer {
  public:
    struct options {
        /// The kernel backlog of connections to buffer.
        int32_t backlog{128};
    };

    explicit server(
            std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler,
            std::shared_ptr<context> tls_ctx,
            const network::socket_address &endpoint,
            options opts = options{
                    .backlog = 128,
            }
    );

    server(const server &) = delete;
    server(server &&other);
    auto operator=(const server &) -> server & = delete;
    auto operator=(server &&other) -> server &;
    ~server() override = default;

    /**
     * Polls for new incoming tcp connections.
     * @param timeout How long to wait for a new connection before timing out, zero waits indefinitely.
     * @return The result of the poll, 'event' means the poll was successful and there is at least 1
     *         connection ready to be accepted.
     */
    auto poll(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) -> silicon::coroutine::task<silicon::coroutine::poll_status> override {
        return m_scheduler->poll(m_accept_socket.native_handle(), silicon::coroutine::poll_op::read, timeout, m_cancel_trigger.get_token());
    }

    /**
     * Accepts an incoming tcp client connection.  On failure the tcp clients socket will be set to
     * and invalid state, use the socket.is_value() to verify the client was correctly accepted.
     * @param timeout The timeout to complete the TLS handshake.
     * @return The newly connected tcp client connection.
     */
    auto accept(std::chrono::milliseconds timeout = std::chrono::seconds{30}) -> silicon::coroutine::task<silicon::network::tls::client> override;

    /**
     * @return The tcp accept socket this server is using.
     * @{
     **/
    [[nodiscard]] auto accept_socket() -> network::socket & { return m_accept_socket; }
    [[nodiscard]] auto accept_socket() const -> const network::socket & { return m_accept_socket; }
    /** @} */

    auto shutdown() {
        m_cancel_trigger.signal_stop();
        m_accept_socket.shutdown(silicon::coroutine::poll_op::read_write);
    }

  private:
    /// The io scheduler for awaiting new connections.
    silicon::scheduler::io_scheduler *m_scheduler{nullptr};
    // The tls context.
    std::shared_ptr<context> m_tls_ctx{nullptr};
    /// The bind and listen options for this server.
    options m_options;
    /// The socket for accepting new tcp connections on.
    network::socket m_accept_socket{-1};
    /// Stop signal to trigger a cancellation of the async accept poll operation.
    poll_stop_source m_cancel_trigger{};
};

} // namespace silicon::network::tls

#endif // #ifdef SILICON_FEATURE_TLS
