module;

#include <expected>

#include <chrono>
#include <coroutine>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include <tuple>
#include <silicon/proxy/proxy_macros.h>

#include <silicon/common.h>
export module silicon.network:tcp;

export import silicon.coroutine;
export import silicon.scheduler;
export import silicon.scheduler.task;
import :facade;
import silicon.proxy;

export namespace silicon::network::tcp {

PRO_DEF_MEM_DISPATCH(MemTcpClientSocket, socket);
PRO_DEF_MEM_DISPATCH(MemTcpClientConnect, connect);
PRO_DEF_MEM_DISPATCH(MemTcpClientPoll, poll);

struct tcp_client_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemTcpClientSocket,
                       network::socket &(),
                       const network::socket &() const>
      ::add_convention<MemTcpClientConnect,
                       silicon::scheduler::task<network::connect_status>(std::chrono::milliseconds)>
      ::add_convention<MemTcpClientPoll,
                       silicon::scheduler::task<silicon::scheduler::poll_status>(
                               silicon::scheduler::poll_op, std::chrono::milliseconds)>
      ::build {};

using tcp_client_proxy = silicon::proxy::proxy<tcp_client_facade>;
using tcp_client_view = silicon::proxy::proxy_view<tcp_client_facade>;

template<class T, class... Args>
[[nodiscard]] tcp_client_proxy make_tcp_client_proxy(Args &&...args) {
    return silicon::proxy::make_proxy<tcp_client_facade, T>(std::forward<Args>(args)...);
}

template<class T>
    requires silicon::proxy::proxiable_target<T, tcp_client_facade>
[[nodiscard]] tcp_client_view make_tcp_client_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<tcp_client_facade>(target);
}

class client;

class server;

class SILICON_CORE_API client final {
  public:

    static auto create(std::unique_ptr<silicon::scheduler::io_scheduler> &, network::socket_address)
            -> network::result<client>;

    client(const client &other);
    client(client &&other) noexcept;
    client & operator=(const client &other) noexcept ;
    client & operator=(client &&other) noexcept ;
    ~client();

    [[nodiscard]] auto socket() -> network::socket &;
    [[nodiscard]] auto socket() const -> const network::socket &;

    auto connect(std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<network::connect_status>;

    template<
            silicon::scheduler::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::mutable_buffer_traits<buffer_type>::element_type>
    auto read_some(buffer_type &buffer, const std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::pair<io_status, std::span<element_type>>> {
        if(buffer.empty()) {
            co_return {io_status{io_status::kind::kOk}, {}};
        }
        auto [status, buf] = co_await read_some_impl(std::as_writable_bytes(std::span{buffer}), timeout);
        co_return {status, std::span<element_type>{reinterpret_cast<element_type *>(buf.data()), buf.size()}};
    }

    template<
            silicon::scheduler::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::mutable_buffer_traits<buffer_type>::element_type>
    auto read_exact(buffer_type &buffer, const std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::pair<io_status, std::span<element_type>>> {
        if(buffer.empty()) {
            co_return {io_status{io_status::kind::kOk}, {}};
        }
        auto [status, buf] = co_await read_exact_impl(std::as_writable_bytes(std::span{buffer}), timeout);
        co_return {status, std::span<element_type>{reinterpret_cast<element_type *>(buf.data()), buf.size()}};
    }

    template<
            silicon::scheduler::concepts::const_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::const_buffer_traits<buffer_type>::element_type>
    auto write_some(const buffer_type &buffer, const std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::pair<io_status, std::span<element_type>>> {
        static_assert(sizeof(element_type) == 1);

        if(buffer.empty()) {
            co_return {io_status{io_status::kind::kOk}, {}};
        }
        auto [status, buf] = co_await write_some_impl(std::as_bytes(std::span{buffer}), timeout);
        co_return {status, std::span<element_type>{reinterpret_cast<element_type *>(buf.data()), buf.size()}};
    }

    template<
            silicon::scheduler::concepts::const_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::const_buffer_traits<buffer_type>::element_type>
    auto write_all(const buffer_type &buffer, const std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::pair<io_status, std::span<element_type>>> {
        static_assert(sizeof(element_type) == 1);

        if(buffer.empty()) {
            co_return {io_status{io_status::kind::kOk}, {}};
        }
        auto [status, buf] = co_await write_all_impl(std::as_bytes(std::span{buffer}), timeout);
        co_return {status, std::span<element_type>{reinterpret_cast<element_type *>(buf.data()), buf.size()}};
    }

  private:
    auto read_some_impl(
            std::span<std::byte>,
            const std::chrono::milliseconds = std::chrono::milliseconds{0}
    )
            -> silicon::scheduler::task<std::pair<io_status, std::span<std::byte>>>;

    auto read_exact_impl(
            std::span<std::byte>,
            const std::chrono::milliseconds = std::chrono::milliseconds{0}
    )
            -> silicon::scheduler::task<std::pair<io_status, std::span<std::byte>>>;

    auto write_some_impl(
            std::span<const std::byte>,
            const std::chrono::milliseconds = std::chrono::milliseconds{0}
    )
            -> silicon::scheduler::task<std::pair<io_status, std::span<const std::byte>>>;

    auto write_all_impl(
            std::span<const std::byte>,
            const std::chrono::milliseconds = std::chrono::milliseconds{0}
    )
            -> silicon::scheduler::task<std::pair<io_status, std::span<const std::byte>>>;

    auto poll(const silicon::scheduler::poll_op, const std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<silicon::scheduler::poll_status>;

    template<
            silicon::scheduler::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::mutable_buffer_traits<buffer_type>::element_type>
    std::pair<io_status, std::span<element_type>> recv(buffer_type &&buffer) ;

    template<
            silicon::scheduler::concepts::const_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::const_buffer_traits<buffer_type>::element_type>
    std::pair<io_status, std::span<element_type>> send(const buffer_type &) ;

    friend server;

    struct impl;
    std::unique_ptr<impl> impl_;

    client(silicon::scheduler::io_scheduler *scheduler, network::socket, const network::socket_address &endpoint);

    client(silicon::scheduler::io_scheduler *scheduler, network::socket_address, network::socket);
};

PRO_DEF_MEM_DISPATCH(MemTcpServerAccept, accept);

struct tcp_server_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemTcpServerAccept,
                       silicon::scheduler::task<std::expected<client, io_status>>(
                               std::chrono::milliseconds)>
      ::build {};

using tcp_server_proxy = silicon::proxy::proxy<tcp_server_facade>;
using tcp_server_view = silicon::proxy::proxy_view<tcp_server_facade>;

template<class T, class... Args>
[[nodiscard]] tcp_server_proxy make_tcp_server_proxy(Args &&...args) {
    return silicon::proxy::make_proxy<tcp_server_facade, T>(std::forward<Args>(args)...);
}

template<class T>
    requires silicon::proxy::proxiable_target<T, tcp_server_facade>
[[nodiscard]] tcp_server_view make_tcp_server_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<tcp_server_facade>(target);
}

class SILICON_CORE_API server final {
  public:
    struct options {

        int32_t backlog{128};
    };

    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &,
            const network::socket_address &,
            options = options{
                    .backlog = 128,
            }
    ) -> network::result<server>;

    server(const server &) = delete;
    server(server &&other);
    server & operator=(const server &) = delete;
    server & operator=(server &&other) ;
    ~server();

    auto accept(std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::expected<network::tcp::client, io_status>>;

    [[nodiscard]] auto accept_socket() -> network::socket &;
    [[nodiscard]] auto accept_socket() const -> const network::socket &;

    auto shutdown();

  private:

    auto poll(std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<silicon::scheduler::poll_status>;

    std::expected<silicon::network::tcp::client, io_status> accept_now() ;

    friend client;

    struct impl;
    std::unique_ptr<impl> impl_;

    server(silicon::scheduler::io_scheduler *scheduler, options, network::socket);
};

}
