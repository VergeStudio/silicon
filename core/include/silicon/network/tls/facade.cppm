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

#include <silicon/common.h>
export module silicon.network:tls;
import silicon.error;

export import silicon.coroutine;
export import silicon.scheduler;
export import silicon.scheduler.task;
import :facade;
import silicon.proxy;
import silicon.time;

#ifdef SILICON_FEATURE_TLS

export namespace silicon::network::tls {

class client;

enum class tls_file_type : int {

    asn1 = SSL_FILETYPE_ASN1,

    pem = SSL_FILETYPE_PEM
};

enum class verify_peer_t : int {
    kYes,
    kNo
};

enum class connection_status {

    kConnected,

    kNotConnected,

    kContextRequired,

    kResourceAllocationFailed,

    kSetFdFailure,

    kHandshakeFailed,

    kTimeout,

    kPollError,

    kUnexpectedClose,

    kInvalidIpAddress,

    kError
};

auto to_string(connection_status) -> const std::string &;

enum class recv_status : int64_t {
    kOk = SSL_ERROR_NONE,

    kBufferIsEmpty = -3,
    kTimeout = -4,

    kCancelled = -5,

    kClosed = SSL_ERROR_ZERO_RETURN,
    kError = SSL_ERROR_SSL,
    kWantRead = SSL_ERROR_WANT_READ,
    kWantWrite = SSL_ERROR_WANT_WRITE,
    kWantConnect = SSL_ERROR_WANT_CONNECT,
    kWantAccept = SSL_ERROR_WANT_ACCEPT,
    kWantX509Lookup = SSL_ERROR_WANT_X509_LOOKUP,
    kErrorSyscall = SSL_ERROR_SYSCALL,

};

auto to_string(recv_status) -> const std::string &;

enum class send_status : int64_t {
    kOk = SSL_ERROR_NONE,

    kBufferIsEmpty = -3,

    kTimeout = -4,

    kCancelled = -5,

    kClosed = SSL_ERROR_ZERO_RETURN,
    kError = SSL_ERROR_SSL,
    kWantRead = SSL_ERROR_WANT_READ,
    kWantWrite = SSL_ERROR_WANT_WRITE,
    kWantConnect = SSL_ERROR_WANT_CONNECT,
    kWantAccept = SSL_ERROR_WANT_ACCEPT,
    kWantX509Lookup = SSL_ERROR_WANT_X509_LOOKUP,
    kErrorSyscall = SSL_ERROR_SYSCALL,

};

auto to_string(send_status) -> const std::string &;

class SILICON_CORE_API context {
  public:

    static auto create(verify_peer_t = verify_peer_t::kYes) -> silicon::error::result<context>;

    static auto create(
            std::filesystem::path,
            tls_file_type,
            std::filesystem::path,
            tls_file_type,
            verify_peer_t = verify_peer_t::kYes
    ) -> silicon::error::result<context>;

    context(const context &) = delete;
    context & operator=(const context &) = delete;
    context(context &&other) noexcept: m_ssl_ctx(std::exchange(other.m_ssl_ctx, nullptr)) {}
    context & operator=(context &&other) noexcept {
        if(std::addressof(other) != this) {
            if(m_ssl_ctx != nullptr) { SSL_CTX_free(m_ssl_ctx); }
            m_ssl_ctx = std::exchange(other.m_ssl_ctx, nullptr);
        }
        return *this;
    }
    ~context();

  private:

    explicit context(SSL_CTX *ssl_ctx) noexcept: m_ssl_ctx(ssl_ctx) {}

    SSL_CTX *m_ssl_ctx{nullptr};

    friend client;

    SSL_CTX * native_handle() { return m_ssl_ctx; }
    const SSL_CTX * native_handle() const { return m_ssl_ctx; }
};

PRO_DEF_MEM_DISPATCH(MemTlsClientConnect, connect);

struct tls_client_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemTlsClientConnect,
                       silicon::scheduler::task<connection_status>(std::chrono::milliseconds)>
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

class server;

class client final {
  public:

    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &,
            std::shared_ptr<context>,
            const network::socket_address &
    ) -> silicon::error::result<client>;

    client(const client &) = delete;
    client(client &&other) noexcept;
    client & operator=(const client &) noexcept = delete;
    client & operator=(client &&other) noexcept ;
    ~client();

    [[nodiscard]] auto socket() -> network::socket & { return m_socket; }
    [[nodiscard]] auto socket() const -> const network::socket & { return m_socket; }

    auto connect(std::chrono::milliseconds = std::chrono::milliseconds{0}) -> silicon::scheduler::task<connection_status>;

    template<
            silicon::scheduler::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::mutable_buffer_traits<buffer_type>::element_type>
    silicon::scheduler::task<std::pair<recv_status, std::span<element_type>>> recv(buffer_type &buffer, std::optional<std::chrono::milliseconds> timeout = std::nullopt) {
        if(buffer.empty()) {
            co_return {recv_status::kBufferIsEmpty, std::span<element_type>{}};
        }

        auto *tls = m_tls_info.m_tls_ptr.get();

        auto op = poll_op::read;

        auto first = true;
        silicon::time::steady_clock::time_point start;
        silicon::time::steady_clock::time_point stop;

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
                start = silicon::time::steady_clock::now();
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
                stop = silicon::time::steady_clock::now();
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

    template<
            silicon::scheduler::concepts::const_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::const_buffer_traits<buffer_type>::element_type>
    silicon::scheduler::task<std::pair<send_status, std::span<element_type>>> send(const buffer_type &buffer, std::optional<std::chrono::milliseconds> timeout = std::nullopt) {

        if(buffer.empty()) {
            co_return {send_status::kBufferIsEmpty, std::span<element_type>{buffer.data(), buffer.size()}};
        }

        auto *tls = m_tls_info.m_tls_ptr.get();

        auto op = poll_op::write;

        auto first = true;
        silicon::time::steady_clock::time_point start;
        silicon::time::steady_clock::time_point stop;

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
                start = silicon::time::steady_clock::now();
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
                stop = silicon::time::steady_clock::now();
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

    silicon::scheduler::task<void> shutdown() {
        co_await shutdown(std::chrono::seconds{30});
    }

    template<typename rep, typename period>
    silicon::scheduler::task<void> shutdown(std::chrono::duration<rep, period> timeout) {

        if(m_shutdown.exchange(true, std::memory_order::acq_rel) != false) {
            co_return;
        }

        if(m_tls_info.m_tls_ptr != nullptr && !m_tls_info.m_tls_error) {
            co_await tls_shutdown_and_free(std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
        }
    }

  private:

    auto handshake(std::chrono::milliseconds = std::chrono::milliseconds{0}) -> silicon::scheduler::task<connection_status>;

    auto poll(silicon::scheduler::poll_op op, std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<poll_status> {
        return m_scheduler->poll(m_socket.native_handle(), op, timeout);
    }

    struct tls_deleter {
        void operator()(SSL *ssl) const { SSL_free(ssl); }
    };

    using tls_unique_ptr = std::unique_ptr<SSL, tls_deleter>;

    enum class tls_connection_type {

        connect,

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

        tls_info & operator=(const tls_info &) noexcept = delete;

        tls_info & operator=(tls_info &&other) noexcept {
            if(std::addressof(other) != this) {
                m_tls_connection_type = std::exchange(other.m_tls_connection_type, tls_connection_type::connect);
                m_tls_ptr = std::move(other.m_tls_ptr);
                m_tls_error = std::exchange(other.m_tls_error, false);
                m_tls_connection_status = std::move(other.m_tls_connection_status);
            }
            return *this;
        }

        tls_connection_type m_tls_connection_type{tls_connection_type::connect};

        tls_unique_ptr m_tls_ptr{nullptr};

        bool m_tls_error{false};

        std::optional<connection_status> m_tls_connection_status{std::nullopt};
    };

    friend server;
    client(silicon::scheduler::io_scheduler *scheduler, std::shared_ptr<context>, network::socket, const network::socket_address &endpoint);

    client(silicon::scheduler::io_scheduler *scheduler, std::shared_ptr<context>, const network::socket_address &endpoint, network::socket);

    silicon::scheduler::io_scheduler *m_scheduler{nullptr};

    std::shared_ptr<context> m_tls_ctx{nullptr};

    network::socket_address m_endpoint;

    network::socket m_socket{-1};

    std::optional<connection_status> m_connect_status{std::nullopt};

    tls_info m_tls_info{};

    std::atomic<bool> m_shutdown{false};

    auto tls_shutdown_and_free(std::chrono::milliseconds = std::chrono::milliseconds{0}) -> silicon::scheduler::task<void>;
};

PRO_DEF_MEM_DISPATCH(MemTlsServerPoll, poll);
PRO_DEF_MEM_DISPATCH(MemTlsServerAccept, accept);

struct tls_server_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemTlsServerPoll,
                       silicon::scheduler::task<silicon::scheduler::poll_status>(
                               std::chrono::milliseconds)>
      ::add_convention<MemTlsServerAccept,
                       silicon::scheduler::task<client>(std::chrono::milliseconds)>
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

        int32_t backlog{128};
    };

    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &,
            std::shared_ptr<context>,
            const network::socket_address &,
            options = options{
                    .backlog = 128,
            }
    ) -> silicon::error::result<server>;

    server(const server &) = delete;
    server(server &&other);
    server & operator=(const server &) = delete;
    server & operator=(server &&other) ;
    ~server() = default;

    auto poll(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) -> silicon::scheduler::task<silicon::scheduler::poll_status> {
        return m_scheduler->poll(m_accept_socket.native_handle(), silicon::scheduler::poll_op::read, timeout, m_cancel_trigger.get_token());
    }

    auto accept(std::chrono::milliseconds = std::chrono::seconds{30}) -> silicon::scheduler::task<silicon::network::tls::client>;

    [[nodiscard]] auto accept_socket() -> network::socket & { return m_accept_socket; }
    [[nodiscard]] auto accept_socket() const -> const network::socket & { return m_accept_socket; }

    auto shutdown() {
        m_cancel_trigger.signal_stop();
        m_accept_socket.shutdown(silicon::scheduler::poll_op::read_write);
    }

  private:

    server(silicon::scheduler::io_scheduler *scheduler, std::shared_ptr<context>, options, network::socket);

    silicon::scheduler::io_scheduler *m_scheduler{nullptr};

    std::shared_ptr<context> m_tls_ctx{nullptr};

    options m_options;

    network::socket m_accept_socket{-1};

    poll_stop_source m_cancel_trigger{};
};

}

#endif
