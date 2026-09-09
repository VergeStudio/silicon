module;

#include <chrono>
#include <coroutine>
#include <memory>
#include <span>

#include <tuple>
#include <silicon/proxy/proxy_macros.h>

#include <silicon/common.h>
export module silicon.network:udp;
import silicon.error;

export import silicon.coroutine;
export import silicon.scheduler;
export import silicon.scheduler.task;
import :facade;
import silicon.proxy;

export namespace silicon::network::udp {

PRO_DEF_MEM_DISPATCH(MemUdpPeerSocket, socket);

struct udp_peer_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemUdpPeerSocket,
                       network::socket &() noexcept,
                       const network::socket &() const noexcept>
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

class SILICON_CORE_API peer final {
  public:

    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &,
            network::domain_t = network::domain_t::kIpv4
    ) -> silicon::error::result<peer>;

    static auto create(
            std::unique_ptr<silicon::scheduler::io_scheduler> &,
            const network::socket_address &
    ) -> silicon::error::result<peer>;

    peer(const peer &) noexcept;
    peer(peer &&) noexcept;
    peer & operator=(const peer &) noexcept ;
    peer & operator=(peer &&) noexcept ;
    ~peer();

    auto socket() noexcept -> network::socket &;

    auto socket() const noexcept -> const network::socket &;

    template<silicon::scheduler::concepts::const_buffer buffer_type>
    auto write_to(
            const socket_address &address,
            const buffer_type &buffer,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0}
    ) -> silicon::scheduler::task<io_status> {
        co_return co_await write_to_impl(address, std::as_bytes(std::span{buffer}), timeout);
    }

    template<silicon::scheduler::concepts::mutable_buffer buffer_type>
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

    auto poll(silicon::scheduler::poll_op, std::chrono::milliseconds = std::chrono::milliseconds{0})
            -> silicon::scheduler::task<silicon::scheduler::poll_status>;

    template<silicon::scheduler::concepts::const_buffer buffer_type>
    auto sendto(const network::socket_address &, const buffer_type &) -> io_status;

    template<
            silicon::scheduler::concepts::mutable_buffer buffer_type,
            typename element_type = typename silicon::scheduler::concepts::mutable_buffer_traits<buffer_type>::element_type>
    std::tuple<io_status, network::socket_address, std::span<element_type>> recvfrom(buffer_type &&buffer) ;

  private:

    peer(silicon::scheduler::io_scheduler *scheduler, network::socket, bool);

    struct impl;
    std::unique_ptr<impl> impl_;
};

}
