// Interface partition silicon.network:udp
//
// UDP peer abstraction. Template read/write methods are kept inline in module
// purview. Pulls :core types and the scheduler task / coroutine primitives.
//
// PIMPL: 所有数据成员位于 udp::peer::impl（定义于实现单元 peer.cpp），
// 接口单元仅前向声明 struct impl 并持有 std::unique_ptr<impl> impl_。

module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <sys/socket.h>
#endif

#include <chrono>
#include <coroutine>
#include <memory>
#include <span>

#include <tuple>
#include <silicon/proxy/proxy_macros.h>

#include <silicon/common.h>
export module silicon.network:udp;

export import silicon.coroutine;
export import silicon.scheduler;
export import silicon.scheduler.task;
import :facade;
import silicon.proxy;

export namespace silicon::network::udp {

/// @brief 类型擦除门面：UDP 对等端的可擦除接口。
///
/// 任何满足下列成员的类型（含 udp::peer）都自动满足该门面，无需继承：
///   network::socket& socket() noexcept;
///   const network::socket& socket() const noexcept;
/// 模板方法（write_to/read_from/sendto/recvfrom）保留在具体类，因 pro·xy 门面无法擦除模板成员。
PRO_DEF_MEM_DISPATCH(MemUdpPeerSocket, socket);

struct udp_peer_facade
    : silicon::proxy::facade_builder                  //
      ::add_convention<MemUdpPeerSocket,             //
                       network::socket &() noexcept, //
                       const network::socket &() const noexcept> //
      ::build {};

using udp_peer_proxy = silicon::proxy::proxy<udp_peer_facade>;
using udp_peer_view = silicon::proxy::proxy_view<udp_peer_facade>;

template<class T, class... Args>
[[nodiscard]] udp_peer_proxy make_udp_peer_proxy(Args &&...args) {
    return silicon::proxy::make_proxy<udp_peer_facade, T>(std::forward<Args>(args)...);
}

template<class T>
    requires silicon::proxy::proxiable_target<T, udp_peer_facade>
[[nodiscard]] udp_peer_view make_udp_peer_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<udp_peer_facade>(target);
}

class CORE_API peer final {
  public:
    /**
     * Creates a udp peer that can send packets but not receive them.  This udp peer will not explicitly
     * bind to a local ip+port.
     *
     * @return 就绪的 peer；scheduler 为空时返回 network_error::kNullScheduler，
     *         套接字创建失败时返回相应错误码。
     */
    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &,
            network::domain_t = network::domain_t::kIpv4
    ) -> network::result<peer>;

    /**
     * Creates a udp peer that can send and receive packets.  This peer will bind to the given ip_port.
     *
     * @return 就绪并已 bind 的 peer；scheduler 为空时返回
     *         network_error::kNullScheduler，bind 失败时返回 kBindFailed。
     */
    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &,
            const network::socket_address &
    ) -> network::result<peer>;

    peer(const peer &) noexcept;
    peer(peer &&) noexcept;
    peer & operator=(const peer &) noexcept ;
    peer & operator=(peer &&) noexcept ;
    ~peer();

    /**
     * @return A reference to the underlying socket.
     */
    auto socket() noexcept -> network::socket &;

    /**
     * @return A const reference to the underlying socket.
     */
    auto socket() const noexcept -> const network::socket &;

    /**
     * @param peer_info The peer to send the data to.
     * @param buffer The data to send.
     * @return The status of operation
     */
    template<silicon::coroutine::concepts::const_buffer buffer_type>
    auto write_to(
            const socket_address &address,
            const buffer_type &buffer,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) -> silicon::scheduler::task<io_status> {
        co_return co_await write_to_impl(address, std::as_bytes(std::span{buffer}), timeout);
    }

    /**
     * @param buffer The buffer to receive data into.
     * @return The receive status, if ok then also the peer who sent the data and the data.
     *         The span view of the data will be set to the size of the received data, this will
     *         always start at the beggining of the buffer but depending on how large the data was
     *         it might not fill the entire buffer.
     */
    template<silicon::coroutine::concepts::mutable_buffer buffer_type>
    auto read_from(buffer_type &buffer, std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::tuple<io_status, socket_address, std::span<std::byte>>> {
        co_return co_await read_from_impl(std::as_writable_bytes(std::span{buffer}), timeout);
    }

  private:
    auto write_to_impl(
            const socket_address &,
            const std::span<const std::byte>,
            std::chrono::milliseconds = std::chrono::milliseconds{0}
    ) -> silicon::scheduler::task<io_status>;

    auto read_from_impl(std::span<std::byte>, std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::tuple<io_status, socket_address, std::span<std::byte>>>;

    auto poll(silicon::coroutine::poll_op, std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<silicon::coroutine::poll_status>;

    /**
     * @param peer_info The peer to send the data to.
     * @param buffer The data to send.
     * @return The status of send call and a span view of any data that wasn't sent.  This data if
     *         un-sent will correspond to bytes at the end of the given buffer.
     */
    template<silicon::coroutine::concepts::const_buffer buffer_type>
    auto sendto(const network::socket_address &, const buffer_type &) -> io_status;

    /**
     * @param buffer The buffer to receive data into.
     * @return The receive status, if ok then also the peer who sent the data and the data.
     *         The span view of the data will be set to the size of the received data, this will
     *         always start at the beggining of the buffer but depending on how large the data was
     *         it might not fill the entire buffer.
     */
    template<
            silicon::coroutine::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::coroutine::concepts::mutable_buffer_traits<buffer_type>::element_type>
    std::tuple<io_status, network::socket_address, std::span<element_type>> recvfrom(buffer_type &&buffer) ;

  private:
    /// create() 专用：所有可失败的前置校验都已在工厂中完成。
    peer(silicon::scheduler::io_scheduler *scheduler, network::socket, bool);

    /// PIMPL 实现体（完整类型定义于 peer.cpp）。
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace silicon::network::udp
