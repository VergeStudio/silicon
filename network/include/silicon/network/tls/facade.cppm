// Interface partition silicon.network:tls
//
// OpenSSL-backed TLS client / server abstractions. Compiled only when
// SILICON_FEATURE_TLS is defined (mirrors the original header guards). The
// module declaration is always present so the file is a valid (empty) module
// unit when TLS is disabled; the primary interface only `export import :tls;`
// inside the same guard, so there is no dangling import.
//
// Template read/write methods are kept inline in module purview. Pulls :core
// types and the scheduler task / coroutine primitives it names.

module;

#ifdef SILICON_FEATURE_TLS
#    include <openssl/bio.h>
#    include <openssl/err.h>
#    include <openssl/pem.h>
#    include <openssl/ssl.h>

#    include <filesystem>
#    include <mutex>
#endif

#include <chrono>
#include <coroutine>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include <tuple>
#include <silicon/proxy/proxy_macros.h>

export module silicon.network:tls;

export import silicon.coroutine;
export import silicon.scheduler;
export import silicon.scheduler.task;
import :facade;
import silicon.proxy;

#ifdef SILICON_FEATURE_TLS

export namespace silicon::network::tls {

class client;

enum class tls_file_type : int {
    /// The file is of type ASN1
    asn1 = SSL_FILETYPE_ASN1,
    /// The file is of type PEM
    pem = SSL_FILETYPE_PEM
};

enum class verify_peer_t : int {
    kYes,
    kNo
};

enum class connection_status {
    /// The tls connection was successful.
    kConnected,
    /// The connection hasn't been established yet, use connect() prior to the handshake().
    kNotConnected,
    /// The connection needs a silicon::network::tls::context to perform the handshake.
    kContextRequired,
    /// The internal ssl memory alocation failed.
    kResourceAllocationFailed,
    /// Attempting to set the connections ssl socket/file descriptor failed.
    kSetFdFailure,
    /// The handshake had an error.
    kHandshakeFailed,
    /// The connection timed out.
    kTimeout,
    /// An error occurred while polling for read or write operations on the socket.
    kPollError,
    /// The socket was unexpectedly closed while attempting the handshake.
    kUnexpectedClose,
    /// The given ip address could not be parsed or is invalid.
    kInvalidIpAddress,
    /// There was an unrecoverable error, use errno to get more information on the specific error.
    kError
};

auto to_string(connection_status status) -> const std::string &;

enum class recv_status : int64_t {
    kOk = SSL_ERROR_NONE,
    // The user provided an 0 length buffer.
    kBufferIsEmpty = -3,
    kTimeout = -4,
    // The operation was cancelled.
    kCancelled = -5,
    /// The peer closed the socket.
    kClosed = SSL_ERROR_ZERO_RETURN,
    kError = SSL_ERROR_SSL,
    kWantRead = SSL_ERROR_WANT_READ,
    kWantWrite = SSL_ERROR_WANT_WRITE,
    kWantConnect = SSL_ERROR_WANT_CONNECT,
    kWantAccept = SSL_ERROR_WANT_ACCEPT,
    kWantX509Lookup = SSL_ERROR_WANT_X509_LOOKUP,
    kErrorSyscall = SSL_ERROR_SYSCALL,

};

auto to_string(recv_status status) -> const std::string &;

enum class send_status : int64_t {
    kOk = SSL_ERROR_NONE,
    // The user provided an 0 length buffer.
    kBufferIsEmpty = -3,
    // The operation timed out.
    kTimeout = -4,
    /// The operation was cancelled.
    kCancelled = -5,
    /// The peer closed the socket.
    kClosed = SSL_ERROR_ZERO_RETURN,
    kError = SSL_ERROR_SSL,
    kWantRead = SSL_ERROR_WANT_READ,
    kWantWrite = SSL_ERROR_WANT_WRITE,
    kWantConnect = SSL_ERROR_WANT_CONNECT,
    kWantAccept = SSL_ERROR_WANT_ACCEPT,
    kWantX509Lookup = SSL_ERROR_WANT_X509_LOOKUP,
    kErrorSyscall = SSL_ERROR_SYSCALL,

};

auto to_string(send_status status) -> const std::string &;

class context {
  public:
    /**
     * Creates a context with no certificate and no private key, maybe useful for testing.
     *
     * OpenSSL 上下文分配可能失败，因此以工厂函数返回 expected 而非抛异常。
     * 需要共享所有权（tls::client / tls::server 接收 shared_ptr<context>）时，
     * 将返回值移入 shared_ptr：`std::make_shared<context>(std::move(*ctx))`。
     *
     * @param verify_peer Should the peer be verified? Defaults to true.
     * @return 就绪的 context；分配失败时返回 network_error::kTlsContextInitFailed。
     */
    static auto create(verify_peer_t verify_peer = verify_peer_t::kYes) -> network::result<context>;

    /**
     * Creates a context with the given certificate and the given private key.
     * @param certificate The location of the certificate file.
     * @param certificate_type See `tls_file_type`.
     * @param private_key The location of the private key file.
     * @param private_key_type See `tls_file_type`.
     * @param verify_peer Should the peer be verified? Defaults to true.
     * @return 就绪的 context；证书 / 私钥加载失败或二者不匹配时分别返回
     *         kTlsCertificateLoadFailed / kTlsPrivateKeyLoadFailed / kTlsKeyMismatch。
     */
    static auto create(
            std::filesystem::path certificate,
            tls_file_type certificate_type,
            std::filesystem::path private_key,
            tls_file_type private_key_type,
            verify_peer_t verify_peer = verify_peer_t::kYes
    ) -> network::result<context>;

    /// 独占持有 SSL_CTX*，只可移动不可拷贝（拷贝会导致重复 SSL_CTX_free）。
    context(const context &) = delete;
    auto operator=(const context &) -> context & = delete;
    context(context &&other) noexcept: m_ssl_ctx(std::exchange(other.m_ssl_ctx, nullptr)) {}
    auto operator=(context &&other) noexcept -> context & {
        if(std::addressof(other) != this) {
            if(m_ssl_ctx != nullptr) { SSL_CTX_free(m_ssl_ctx); }
            m_ssl_ctx = std::exchange(other.m_ssl_ctx, nullptr);
        }
        return *this;
    }
    ~context();

  private:
    /// create() 专用：接管一个已初始化完成的 SSL_CTX*。
    explicit context(SSL_CTX *ssl_ctx) noexcept: m_ssl_ctx(ssl_ctx) {}

    SSL_CTX *m_ssl_ctx{nullptr};

    /// The following classes use the underlying SSL_CTX* object for performing SSL functions.
    friend client;

    auto native_handle() -> SSL_CTX * { return m_ssl_ctx; }
    auto native_handle() const -> const SSL_CTX * { return m_ssl_ctx; }
};

/// @brief 类型擦除门面：TLS 客户端的可擦除接口。
///
/// 任何满足下列成员的类型（含 tls::client）都自动满足该门面，无需继承：
///   silicon::scheduler::task<connection_status> connect(std::chrono::milliseconds);
PRO_DEF_MEM_DISPATCH(MemTlsClientConnect, connect);

struct tls_client_facade
    : silicon::proxy::facade_builder                                                  //
      ::add_convention<MemTlsClientConnect,                                          //
                       silicon::scheduler::task<connection_status>(std::chrono::milliseconds)> //
      ::build {};

using tls_client_proxy = silicon::proxy::proxy<tls_client_facade>;
using tls_client_view = silicon::proxy::proxy_view<tls_client_facade>;

template<class T, class... Args>
[[nodiscard]] tls_client_proxy make_tls_client_proxy(Args &&...args) {
    return silicon::proxy::make_proxy<tls_client_facade, T>(std::forward<Args>(args)...);
}

template<class T>
    requires silicon::proxy::proxiable_target<T, tls_client_facade>
[[nodiscard]] tls_client_view make_tls_client_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<tls_client_facade>(target);
}

/// @brief TLS 服务端门面（tls_server_facade）定义见本文件下方 client 完整声明之后，
///        因其 accept 返回类型引用 tls::client，需待 client 完整声明后方可命名。

class server;

class client final {
  public:
    /**
     * Creates a new tls client that can connect to an ip address + port. By default, the socket
     * created will be in non-blocking mode, meaning that any sending or receiving of data should
     * be polled for event readiness prior.
     *
     * 构造过程可能失败（空 scheduler / 空 tls_ctx / 套接字创建失败），因此以工厂函数
     * 返回 expected 而非抛异常。
     *
     * @param scheduler The io scheduler to drive the tls client.
     * @param tls_ctx The tls context.
     * @param endpoint The remote address this client will connect to.
     * @return 就绪的 client；分别在空 scheduler / 空 tls_ctx 时返回
     *         network_error::kNullScheduler / kNullTlsContext。
     */
    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler,
            std::shared_ptr<context> tls_ctx,
            const network::socket_address &endpoint
    ) -> network::result<client>;

    client(const client &) = delete;
    client(client &&other) noexcept;
    auto operator=(const client &) noexcept -> client & = delete;
    auto operator=(client &&other) noexcept -> client &;
    ~client();

    /**
     * @return The tcp socket this client is using.
     * @{
     **/
    [[nodiscard]] auto socket() -> network::socket & { return m_socket; }
    [[nodiscard]] auto socket() const -> const network::socket & { return m_socket; }
    /** @} */

    /**
     * Connects to the address+port with the given timeout and completes the tls handshake.
     * Once connected calling this function only returns the connected status, it will not reconnect.
     * @param timeout How long to wait for the connection to establish? Timeout of zero is indefinite.
     * @return The result status of trying to connect.
     */
    auto connect(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) -> silicon::scheduler::task<connection_status>;

    /**
     * Receives incoming data into the given buffer. This function will automatically poll for readability.
     * @param buffer Received bytes are written into this buffer up to the buffers size.
     * @param timeout The amount of time to wait for data to arrive.
     * @return The status of the recv call and a span of the bytes received (if any). The span of
     *         bytes will be a subspan or full span of the given input buffer.
     */
    template<
            silicon::coroutine::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::mutable_buffer_traits<buffer_type>::element_type>
    auto recv(buffer_type &buffer, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
            -> silicon::scheduler::task<std::pair<recv_status, std::span<element_type>>> {
        if(buffer.empty()) {
            co_return {recv_status::kBufferIsEmpty, std::span<element_type>{}};
        }

        auto *tls = m_tls_info.m_tls_ptr.get();

        auto op = poll_op::read;

        auto first = true;
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point stop;

        while(true) {
            if(timeout.has_value()) {
                auto &t = timeout.value();
                if(!first) {
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                    t -= duration;
                    if(t <= std::chrono::microseconds{0}) {
                        co_return {recv_status::kTimeout, std::span<element_type>{}};
                    }
                }

                first = false;
                start = std::chrono::steady_clock::now();
            }

            auto pstatus = co_await poll(op, timeout.value_or(std::chrono::milliseconds{0}));
            switch(pstatus) {
                case poll_status::read:
                    break;
                case poll_status::write:
                    continue;
                case poll_status::timeout:
                    co_return {recv_status::kTimeout, std::span<element_type>{}};
                case poll_status::error:
                    co_return {recv_status::kError, std::span<element_type>{}};
                case poll_status::closed:
                    co_return {recv_status::kClosed, std::span<element_type>{}};
                case poll_status::cancelled:
                    co_return {recv_status::kCancelled, std::span<element_type>{}};
            }

            size_t bytes_recv{0};
            ERR_clear_error();
            int r = SSL_read_ex(tls, buffer.data(), buffer.size(), &bytes_recv);
            if(timeout.has_value()) {
                stop = std::chrono::steady_clock::now();
            }
            if(r <= 0) {
                int err = SSL_get_error(tls, r);
                if(err == SSL_ERROR_WANT_READ) {
                    op = poll_op::read;
                    continue;
                } else if(err == SSL_ERROR_WANT_WRITE) {
                    op = poll_op::write;
                    continue;
                } else {
                    co_return {static_cast<recv_status>(err), std::span<element_type>{}};
                }
            } else {
                co_return {recv_status::kOk, std::span<element_type>{buffer.data(), static_cast<size_t>(bytes_recv)}};
            }
        }
    }

    /**
     * Sends outgoing data from the given buffer. If a partial write occurs then the returned span will
     * contain a view into the unsent bytes. This function will automatically call for write-ability on the socket.
     * @param buffer The data to write on the tls socket.
     * @param timeout The amount of time to send the data before timing out.
     * @return The status of the send call and a span of any remaining bytes not sent. If all bytes
     *         were successfully sent the status will be 'ok' and the remaining span will be empty.
     */
    template<
            silicon::coroutine::concepts::const_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::const_buffer_traits<buffer_type>::element_type>
    auto send(const buffer_type &buffer, std::optional<std::chrono::milliseconds> timeout = std::nullopt)
            -> silicon::scheduler::task<std::pair<send_status, std::span<element_type>>> {
        // Make sure there is data to send.
        if(buffer.empty()) {
            co_return {send_status::kBufferIsEmpty, std::span<element_type>{buffer.data(), buffer.size()}};
        }

        auto *tls = m_tls_info.m_tls_ptr.get();

        auto op = poll_op::write;

        auto first = true;
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point stop;

        while(true) {
            if(timeout.has_value()) {
                auto &t = timeout.value();
                if(!first) {
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
                    t -= duration;
                    if(t <= std::chrono::microseconds{0}) {
                        co_return {send_status::kTimeout, std::span<element_type>{}};
                    }
                }

                first = false;
                start = std::chrono::steady_clock::now();
            }

            auto pstatus = co_await poll(op, timeout.value_or(std::chrono::milliseconds{0}));
            switch(pstatus) {
                case poll_status::write:
                    break;
                case poll_status::read:
                    continue;
                case poll_status::timeout:
                    co_return {send_status::kTimeout, std::span<element_type>{}};
                case poll_status::error:
                    co_return {send_status::kError, std::span<element_type>{}};
                case poll_status::closed:
                    co_return {send_status::kClosed, std::span<element_type>{}};
                case poll_status::cancelled:
                    co_return {send_status::kCancelled, std::span<element_type>{}};
            }

            size_t bytes_sent{0};
            ERR_clear_error();
            int r = SSL_write_ex(tls, buffer.data(), buffer.size(), &bytes_sent);
            if(timeout.has_value()) {
                stop = std::chrono::steady_clock::now();
            }
            if(r <= 0) {
                int err = SSL_get_error(tls, r);

                if(err == SSL_ERROR_WANT_WRITE) {
                    op = poll_op::write;
                    continue;
                } else if(err == SSL_ERROR_WANT_READ) {
                    op = poll_op::read;
                    continue;
                } else {
                    co_return {static_cast<send_status>(err), std::span<element_type>{}};
                }
            } else {
                co_return {
                        send_status::kOk, std::span<element_type>{buffer.data() + bytes_sent, buffer.size() - bytes_sent}
                };
            }
        }
    }

    /**
     * Shuts down the tls client with a 30 second timeout.
     *
     * IMPORTANT:
     * This is very important to call manually for each client as the client's destructor will synchronously wait
     * to shutdown if this isn't called before the client is destructed, possibly hanging the thread of execution
     * until it completes.
     * @return Task.
     */
    auto shutdown() -> silicon::scheduler::task<void> {
        co_await shutdown(std::chrono::seconds{30});
    }

    template<typename rep, typename period>
    auto shutdown(std::chrono::duration<rep, period> timeout) -> silicon::scheduler::task<void> {
        // Only allow the client to be shutdown once.
        if(m_shutdown.exchange(true, std::memory_order::acq_rel) != false) {
            co_return;
        }

        // If the client exists and it didn't have an error.
        if(m_tls_info.m_tls_ptr != nullptr && !m_tls_info.m_tls_error) {
            co_await tls_shutdown_and_free(std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
        }
    }

  private:
    /**
     * @param timeout How long to allow for the tls handshake to successfully complete?
     * @return The result of the tls handshake.
     */
    auto handshake(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) -> silicon::scheduler::task<connection_status>;

    /**
     * Polls for the given operation on this client's socket.  This should be done prior to
     * calling recv and after a send that doesn't send the entire buffer.
     * @param op The poll operation to perform, use read for incoming data and write for outgoing.
     * @param timeout The amount of time to wait for the poll event to be ready.  Use zero for infinte timeout.
     * @return The status result of th poll operation.  When poll_status::read or poll_status::write is returned then
     *         this specific event operation is ready.
     */
    auto poll(silicon::coroutine::poll_op op, std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<poll_status> {
        return m_scheduler->poll(m_socket.native_handle(), op, timeout);
    }

    struct tls_deleter {
        auto operator()(SSL *ssl) const -> void { SSL_free(ssl); }
    };

    using tls_unique_ptr = std::unique_ptr<SSL, tls_deleter>;

    enum class tls_connection_type {
        /// This connection is a client connecting to a server.
        connect,
        /// This connection is an accepted connection on a sever.
        accept
    };

    struct tls_info {
        tls_info() {}
        explicit tls_info(tls_connection_type type): m_tls_connection_type(type) {}
        tls_info(const tls_info &) noexcept = delete;
        tls_info(tls_info &&other) noexcept
            : m_tls_connection_type(std::exchange(other.m_tls_connection_type, tls_connection_type::connect)),
              m_tls_ptr(std::move(other.m_tls_ptr)),
              m_tls_error(std::exchange(other.m_tls_error, false)),
              m_tls_connection_status(std::move(other.m_tls_connection_status)) {
        }

        auto operator=(const tls_info &) noexcept -> tls_info & = delete;

        auto operator=(tls_info &&other) noexcept -> tls_info & {
            if(std::addressof(other) != this) {
                m_tls_connection_type = std::exchange(other.m_tls_connection_type, tls_connection_type::connect);
                m_tls_ptr = std::move(other.m_tls_ptr);
                m_tls_error = std::exchange(other.m_tls_error, false);
                m_tls_connection_status = std::move(other.m_tls_connection_status);
            }
            return *this;
        }

        /// What kind of connection is this, client initiated connect or server side accept?
        tls_connection_type m_tls_connection_type{tls_connection_type::connect};
        /// OpenSSL ssl connection.
        tls_unique_ptr m_tls_ptr{nullptr};
        /// Was there an error with the SSL/TLS connection?
        bool m_tls_error{false};
        /// The result of the tls connection and handshake.
        std::optional<connection_status> m_tls_connection_status{std::nullopt};
    };

    /// The tls::server creates already connected clients and provides a tcp socket pre-built.
    friend server;
    client(silicon::scheduler::io_scheduler *scheduler, std::shared_ptr<context> tls_ctx, network::socket socket, const network::socket_address &endpoint);

    /// create() 专用：所有可失败的前置校验都已在工厂中完成（注意与上面 server 侧
    /// 构造的形参顺序不同：此处为 endpoint 在前、socket 在后，且不预置 connect 状态）。
    client(silicon::scheduler::io_scheduler *scheduler, std::shared_ptr<context> tls_ctx, const network::socket_address &endpoint, network::socket sock);

    /// The scheduler that will drive this tcp client.
    silicon::scheduler::io_scheduler *m_scheduler{nullptr};
    // The tls context.
    std::shared_ptr<context> m_tls_ctx{nullptr};
    /// Options for what server to connect to.
    network::socket_address m_endpoint;
    /// The tcp socket.
    network::socket m_socket{-1};
    /// Cache the status of the connect in the event the user calls connect() again.
    std::optional<connection_status> m_connect_status{std::nullopt};
    /// SSL/TLS specific information.
    tls_info m_tls_info{};
    /// Flag to signal if this tls client has already been shutdown or not.
    std::atomic<bool> m_shutdown{false};

    auto tls_shutdown_and_free(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) -> silicon::scheduler::task<void>;
};

/// @brief 类型擦除门面：TLS 服务端的可擦除接口。
///
/// 任何满足下列成员的类型（含 tls::server）都自动满足该门面，无需继承：
///   silicon::scheduler::task<silicon::coroutine::poll_status> poll(std::chrono::milliseconds);
///   silicon::scheduler::task<client> accept(std::chrono::milliseconds);
/// 因 accept 返回类型引用 tls::client，本门面定义于 client 完整声明之后。
PRO_DEF_MEM_DISPATCH(MemTlsServerPoll, poll);
PRO_DEF_MEM_DISPATCH(MemTlsServerAccept, accept);

struct tls_server_facade
    : silicon::proxy::facade_builder                                                          //
      ::add_convention<MemTlsServerPoll,                                                      //
                       silicon::scheduler::task<silicon::coroutine::poll_status>(
                               std::chrono::milliseconds)>                                     //
      ::add_convention<MemTlsServerAccept,                                                    //
                       silicon::scheduler::task<client>(std::chrono::milliseconds)>           //
      ::build {};

using tls_server_proxy = silicon::proxy::proxy<tls_server_facade>;
using tls_server_view = silicon::proxy::proxy_view<tls_server_facade>;

template<class T, class... Args>
[[nodiscard]] tls_server_proxy make_tls_server_proxy(Args &&...args) {
    return silicon::proxy::make_proxy<tls_server_facade, T>(std::forward<Args>(args)...);
}

template<class T>
    requires silicon::proxy::proxiable_target<T, tls_server_facade>
[[nodiscard]] tls_server_view make_tls_server_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<tls_server_facade>(target);
}

class server final {
  public:
    struct options {
        /// The kernel backlog of connections to buffer.
        int32_t backlog{128};
    };

    /**
     * Creates a listening tls server bound to the given endpoint.
     *
     * @return 就绪的 server；分别在空 scheduler / 空 tls_ctx 时返回
     *         network_error::kNullScheduler / kNullTlsContext；
     *         bind/listen 失败时返回 kBindFailed / kListenFailed。
     */
    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler,
            std::shared_ptr<context> tls_ctx,
            const network::socket_address &endpoint,
            options opts = options{
                    .backlog = 128,
            }
    ) -> network::result<server>;

    server(const server &) = delete;
    server(server &&other);
    auto operator=(const server &) -> server & = delete;
    auto operator=(server &&other) -> server &;
    ~server() = default;

    /**
     * Polls for new incoming tcp connections.
     * @param timeout How long to wait for a new connection before timing out, zero waits indefinitely.
     * @return The result of the poll, 'event' means the poll was successful and there is at least 1
     *         connection ready to be accepted.
     */
    auto poll(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) -> silicon::scheduler::task<silicon::coroutine::poll_status> {
        return m_scheduler->poll(m_accept_socket.native_handle(), silicon::coroutine::poll_op::read, timeout, m_cancel_trigger.get_token());
    }

    /**
     * Accepts an incoming tcp client connection.  On failure the tcp clients socket will be set to
     * and invalid state, use the socket.is_value() to verify the client was correctly accepted.
     * @param timeout The timeout to complete the TLS handshake.
     * @return The newly connected tcp client connection.
     */
    auto accept(std::chrono::milliseconds timeout = std::chrono::seconds{30}) -> silicon::scheduler::task<silicon::network::tls::client>;

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
    /// create() 专用：所有可失败的前置校验都已在工厂中完成。
    server(silicon::scheduler::io_scheduler *scheduler, std::shared_ptr<context> tls_ctx, options opts, network::socket accept_socket);

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
