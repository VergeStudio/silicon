// Interface partition silicon.network:tcp
//
// TCP client / server abstractions. Concrete classes (tcp::client,
// tcp::server) hide their implementation behind a pImpl `impl` struct so the
// public surface (including the read/write template methods, which must remain
// instantiation-visible in this interface unit) no longer leaks data members.
// The template methods call non-template `*_impl` members that are defined in
// the implementation units, where `impl` is a complete type.

module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <fcntl.h>
#    include <sys/socket.h>
#endif

#include <chrono>
#include <coroutine>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include <tuple>
#include <silicon/proxy/proxy_macros.h>

export module silicon.network:tcp;

export import silicon.coroutine;
export import silicon.scheduler;
export import silicon.scheduler.task;
import :facade;
import silicon.proxy;

export namespace silicon::network::tcp {

/// @brief 类型擦除门面：TCP 客户端的可擦除接口。
///
/// 任何满足下列成员的类型（含 tcp::client）都自动满足该门面，无需继承：
///   network::socket& socket();
///   const network::socket& socket() const;
///   silicon::scheduler::task<network::connect_status> connect(std::chrono::milliseconds);
///   silicon::scheduler::task<silicon::coroutine::poll_status> poll(poll_op, std::chrono::milliseconds);
/// 模板方法（read_some/read_exact/write_some/write_all/recv/send）保留在具体类，
/// 因 pro·xy 门面无法擦除模板成员。
PRO_DEF_MEM_DISPATCH(MemTcpClientSocket, socket);
PRO_DEF_MEM_DISPATCH(MemTcpClientConnect, connect);
PRO_DEF_MEM_DISPATCH(MemTcpClientPoll, poll);

struct tcp_client_facade
    : silicon::proxy::facade_builder                                         //
      ::add_convention<MemTcpClientSocket,                                  //
                       network::socket &(),                                 //
                       const network::socket &() const>                     //
      ::add_convention<MemTcpClientConnect,                                 //
                       silicon::scheduler::task<network::connect_status>(std::chrono::milliseconds)> //
      ::add_convention<MemTcpClientPoll,                                    //
                       silicon::scheduler::task<silicon::coroutine::poll_status>(
                               silicon::coroutine::poll_op, std::chrono::milliseconds)> //
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

/// @brief TCP 服务端门面（tcp_server_facade）定义见本文件下方 client 完整声明之后，
///        因其 accept 返回类型引用 tcp::client，需待 client 完整声明后方可命名。
class client;

class server;

class client final {
  public:
    /**
     * Creates a new tcp client that can connect to an ip address + port.
     *
     * 构造过程可能失败（空 scheduler / 套接字创建失败），因此以工厂函数返回
     * expected 而非抛异常。
     *
     * @param scheduler The io scheduler to drive the tcp client.
     * @param endpoint The remote address this client will connect to.
     * @return 就绪的 client；scheduler 为空时返回 network_error::kNullScheduler，
     *         endpoint 地址族非法或套接字创建失败时返回相应错误码。
     */
    static auto create(std::unique_ptr<silicon::scheduler::io_scheduler> &, network::socket_address)
            -> network::result<client>;

    client(const client &other);
    client(client &&other) noexcept;
    client & operator=(const client &other) noexcept ;
    client & operator=(client &&other) noexcept ;
    ~client();

    /**
     * @return The tcp socket this client is using.
     * @{
     **/
    [[nodiscard]] auto socket() -> network::socket &;
    [[nodiscard]] auto socket() const -> const network::socket &;
    /** @} */

    /**
     * Connects to the address+port with the given timeout. Once connected calling this function
     * only returns the connected status, it will not reconnect.
     * @param timeout How long to wait for the connection to establish? Timeout of zero is indefinite.
     * @return The result status of trying to connect.
     */
    auto connect(std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<network::connect_status>;

    /**
     * Attempts to asynchronously read data from the socket into the provided buffer.
     * @see read_exact()
     * @param buffer Destination buffer to read data into.
     * @param timeout Maximum time to wait for the socket to become readable (0 = infinite).
     * @return A pair of: status of the operation; span pointing to the read part of buffer.
     */
    template<
            silicon::coroutine::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::mutable_buffer_traits<buffer_type>::element_type>
    auto read_some(buffer_type &buffer, const std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::pair<io_status, std::span<element_type>>> {
        if(buffer.empty()) {
            co_return {io_status{io_status::kind::kOk}, {}};
        }
        auto [status, buf] = co_await read_some_impl(std::as_writable_bytes(std::span{buffer}), timeout);
        co_return {status, std::span<element_type>{reinterpret_cast<element_type *>(buf.data()), buf.size()}};
    }

    /**
     * Asynchronously reads data from the socket into the provided buffer, repeatedly invoking
     * read_some() until the whole buffer is filled or an error/timeout occurs.
     * @see read_some()
     * @param buffer Destination buffer to read data into.
     * @param timeout Maximum total time allowed for the operation (0 = infinite).
     * @return A pair of: status of the operation; span pointing to the read part of buffer.
     */
    template<
            silicon::coroutine::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::mutable_buffer_traits<buffer_type>::element_type>
    auto read_exact(buffer_type &buffer, const std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::pair<io_status, std::span<element_type>>> {
        if(buffer.empty()) {
            co_return {io_status{io_status::kind::kOk}, {}};
        }
        auto [status, buf] = co_await read_exact_impl(std::as_writable_bytes(std::span{buffer}), timeout);
        co_return {status, std::span<element_type>{reinterpret_cast<element_type *>(buf.data()), buf.size()}};
    }

    /**
     * Attempts to asynchronously write data from the provided buffer to the socket.
     * @see write_all()
     * @param buffer Buffer containing the data to write.
     * @param timeout Maximum time to wait for the socket to become writable (0 = infinite).
     * @return A pair of: status of the operation; span pointing to the unsent portion of the buffer.
     */
    template<
            silicon::coroutine::concepts::const_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::const_buffer_traits<buffer_type>::element_type>
    auto write_some(const buffer_type &buffer, const std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::pair<io_status, std::span<element_type>>> {
        static_assert(sizeof(element_type) == 1);

        if(buffer.empty()) {
            co_return {io_status{io_status::kind::kOk}, {}};
        }
        auto [status, buf] = co_await write_some_impl(std::as_bytes(std::span{buffer}), timeout);
        co_return {status, std::span<element_type>{reinterpret_cast<element_type *>(buf.data()), buf.size()}};
    }

    /**
     * Asynchronously writes the entire contents of the provided buffer to the socket, repeatedly
     * invoking write_some() until all bytes are sent or an error/timeout occurs.
     * @see write_some()
     * @param buffer The data to write to the socket.
     * @param timeout Maximum total time allowed for the operation (0 = infinite).
     * @return A pair of: status of the operation; span pointing to the unsent portion of the buffer.
     */
    template<
            silicon::coroutine::concepts::const_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::const_buffer_traits<buffer_type>::element_type>
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
            std::span<std::byte> buffer,
            const std::chrono::milliseconds = std::chrono::milliseconds{0}
    )
            -> silicon::scheduler::task<std::pair<io_status, std::span<std::byte>>>;

    auto read_exact_impl(
            std::span<std::byte> buffer,
            const std::chrono::milliseconds = std::chrono::milliseconds{0}
    )
            -> silicon::scheduler::task<std::pair<io_status, std::span<std::byte>>>;

    auto write_some_impl(
            std::span<const std::byte> buffer,
            const std::chrono::milliseconds = std::chrono::milliseconds{0}
    )
            -> silicon::scheduler::task<std::pair<io_status, std::span<const std::byte>>>;

    auto write_all_impl(
            std::span<const std::byte> buffer,
            const std::chrono::milliseconds = std::chrono::milliseconds{0}
    )
            -> silicon::scheduler::task<std::pair<io_status, std::span<const std::byte>>>;

    auto poll(const silicon::coroutine::poll_op, const std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<silicon::coroutine::poll_status>;

    template<
            silicon::coroutine::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::mutable_buffer_traits<buffer_type>::element_type>
    std::pair<io_status, std::span<element_type>> recv(buffer_type &&buffer) ;

    template<
            silicon::coroutine::concepts::const_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::const_buffer_traits<buffer_type>::element_type>
    std::pair<io_status, std::span<element_type>> send(const buffer_type &) ;

    /// The tcp::server creates already connected clients and provides a tcp socket pre-built.
    friend server;

    /// pImpl: all data members live in client::impl (defined in the implementation unit).
    struct impl;
    std::unique_ptr<impl> impl_;

    /// The tcp::server creates already connected clients and provides a tcp socket pre-built.
    client(silicon::scheduler::io_scheduler *scheduler, network::socket, const network::socket_address &endpoint);

    /// create() 专用：所有可失败的前置校验都已在工厂中完成。
    client(silicon::scheduler::io_scheduler *scheduler, network::socket_address, network::socket);
};

/// @brief 类型擦除门面：TCP 服务端的可擦除接口。
///
/// 任何满足下列成员的类型（含 tcp::server）都自动满足该门面，无需继承：
///   silicon::scheduler::task<silicon::coroutine::expected<client, io_status>> accept(std::chrono::milliseconds);
/// 因 accept 返回类型引用 tcp::client，本门面定义于 client 完整声明之后。
PRO_DEF_MEM_DISPATCH(MemTcpServerAccept, accept);

struct tcp_server_facade
    : silicon::proxy::facade_builder                                                       //
      ::add_convention<MemTcpServerAccept,                                                 //
                       silicon::scheduler::task<silicon::coroutine::expected<client, io_status>>(
                               std::chrono::milliseconds)>                                  //
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

class server final {
  public:
    struct options {
        /// The kernel backlog of connections to buffer.
        int32_t backlog{128};
    };

    /**
     * Creates a listening tcp server bound to the given endpoint.
     *
     * 构造过程可能失败（空 scheduler / bind / listen 失败），因此以工厂函数返回
     * expected 而非抛异常。
     *
     * @return 就绪的 server；scheduler 为空时返回 network_error::kNullScheduler，
     *         bind/listen 失败时返回 kBindFailed / kListenFailed 等错误码。
     */
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

    /**
     * Asynchronously waits for an incoming TCP connection and accepts it.
     *
     * @param timeout How long to wait for a new connection before timing out, zero waits indefinitely.
     * @return The newly connected tcp client connection on success or an io_status describing the failure.
     */
    auto accept(std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<silicon::coroutine::expected<network::tcp::client, io_status>>;

    /**
     * @return The tcp accept socket this server is using.
     * @{
     **/
    [[nodiscard]] auto accept_socket() -> network::socket &;
    [[nodiscard]] auto accept_socket() const -> const network::socket &;
    /** @} */

    auto shutdown();

  private:
    /**
     * Polls for new incoming tcp connections.
     * @param timeout How long to wait for a new connection before timing out, zero waits indefinitely.
     * @return The result of the poll, 'event' means the poll was successful and there is at least 1
     *         connection ready to be accepted.
     */
    auto poll(std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<coroutine::poll_status>;

    /**
     * Accepts an incoming tcp client connection.
     * @return The newly connected tcp client connection.
     */
    silicon::coroutine::expected<silicon::network::tcp::client, io_status> accept_now() ;

    friend client;

    /// pImpl: all data members live in server::impl (defined in the implementation unit).
    struct impl;
    std::unique_ptr<impl> impl_;

    /// create() 专用：所有可失败的前置校验都已在工厂中完成。
    server(silicon::scheduler::io_scheduler *scheduler, options, network::socket);
};

} // namespace silicon::network::tcp
