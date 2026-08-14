// Interface partition silicon.network:udp
//
// UDP peer abstraction. Template read/write methods are kept inline in module
// purview. Pulls :core types and the scheduler task / coroutine primitives.
//
// PIMPL: 所有数据成员位于 udp::peer::Impl（定义于实现单元 peer.cpp），
// 接口单元仅前向声明 struct Impl 并持有 std::unique_ptr<Impl> impl_。

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

export module silicon.network:udp;

export import silicon.coroutine;
export import silicon.scheduler;
export import silicon.scheduler.task;
import :core;

export namespace silicon::network::udp {

/// @brief Abstract interface for a UDP peer.
class i_udp_peer {
  public:
    i_udp_peer() = default;
    i_udp_peer(const i_udp_peer &) = delete;
    i_udp_peer(i_udp_peer &&) = delete;
    auto operator=(const i_udp_peer &) -> i_udp_peer & = delete;
    auto operator=(i_udp_peer &&) -> i_udp_peer & = delete;
    virtual ~i_udp_peer() = default;

    virtual auto socket() noexcept -> network::socket & = 0;
    virtual auto socket() const noexcept -> const network::socket & = 0;
};

class peer final: public i_udp_peer {
  public:
    /**
     * Creates a udp peer that can send packets but not receive them.  This udp peer will not explicitly
     * bind to a local ip+port.
     *
     * @return 就绪的 peer；scheduler 为空时返回 network_error::kNullScheduler，
     *         套接字创建失败时返回相应错误码。
     */
    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler,
            network::domain_t domain = network::domain_t::kIpv4
    ) -> network::result<peer>;

    /**
     * Creates a udp peer that can send and receive packets.  This peer will bind to the given ip_port.
     *
     * @return 就绪并已 bind 的 peer；scheduler 为空时返回
     *         network_error::kNullScheduler，bind 失败时返回 kBindFailed。
     */
    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &scheduler,
            const network::socket_address &endpoint
    ) -> network::result<peer>;

    peer(const peer &) noexcept;
    peer(peer &&) noexcept;
    auto operator=(const peer &) noexcept -> peer &;
    auto operator=(peer &&) noexcept -> peer &;
    ~peer() override;

    /**
     * @return A reference to the underlying socket.
     */
    auto socket() noexcept -> network::socket & override;

    /**
     * @return A const reference to the underlying socket.
     */
    auto socket() const noexcept -> const network::socket & override;

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
            const socket_address &address,
            const std::span<const std::byte> buffer,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) -> silicon::scheduler::task<io_status>;

    auto read_from_impl(std::span<std::byte> buffer, std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<std::tuple<io_status, socket_address, std::span<std::byte>>>;

    auto poll(silicon::coroutine::poll_op op, std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<silicon::coroutine::poll_status>;

    /**
     * @param peer_info The peer to send the data to.
     * @param buffer The data to send.
     * @return The status of send call and a span view of any data that wasn't sent.  This data if
     *         un-sent will correspond to bytes at the end of the given buffer.
     */
    template<silicon::coroutine::concepts::const_buffer buffer_type>
    auto sendto(const network::socket_address &endpoint, const buffer_type &buffer) -> io_status;

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
    auto recvfrom(buffer_type &&buffer) -> std::tuple<io_status, network::socket_address, std::span<element_type>>;

  private:
    /// create() 专用：所有可失败的前置校验都已在工厂中完成。
    peer(silicon::scheduler::io_scheduler *scheduler, network::socket sock, bool bound);

    /// PIMPL 实现体（完整类型定义于 peer.cpp）。
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace silicon::network::udp
