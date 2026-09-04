







module;

#include <ares.h>
#if defined(SILICON_PLATFORM_WINDOWS)
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <netdb.h>
#endif

#include <array>
#include <chrono>
#include <coroutine>
#include <expected>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <system_error>
#include <unordered_map>
#include <vector>

export module silicon.network:dns;

export import silicon.coroutine;
export import silicon.scheduler;
export import silicon.scheduler.task;
import :facade;

export namespace silicon::network::dns {



extern uint64_t m_ares_count;

extern std::mutex m_ares_mutex;


template<silicon::scheduler::concepts::io_executor executor_type>
class resolver;

enum class status {
    kComplete,
    kError
};

template<silicon::scheduler::concepts::io_executor executor_type>
class result {
    friend resolver<executor_type>;

  public:
    result(std::unique_ptr<executor_type> &executor, silicon::coroutine::event &resume, uint64_t pending_dns_requests)
        : m_executor(executor),
          m_resume(resume),
          m_pending_dns_requests(pending_dns_requests) {
    }
    ~result() = default;

    
    auto status() const -> dns::status { return m_status; }

    
    auto ip_addresses() const -> const std::vector<silicon::network::ip_address> & { return m_ip_addresses; }

  private:
    std::unique_ptr<executor_type> &m_executor;
    silicon::coroutine::event &m_resume;
    uint64_t m_pending_dns_requests{0};
    dns::status m_status{dns::status::kComplete};
    std::vector<silicon::network::ip_address> m_ip_addresses{};

    friend void ares_dns_callback(void *, int, int, ares_addrinfo *) ;
};

template<silicon::scheduler::concepts::io_executor executor_type>
class resolver {
  public:
    
    static std::expected<std::unique_ptr<resolver>, std::error_code> create(std::unique_ptr<executor_type> &executor, std::chrono::milliseconds timeout) {
        if(executor == nullptr) {
            return std::unexpected(make_error_code(network_error::kNullExecutor));
        }

        {
            std::scoped_lock g{m_ares_mutex};
            if(m_ares_count == 0) {
                auto ares_status = ares_library_init(ARES_LIB_INIT_ALL);
                if(ares_status != ARES_SUCCESS) {
                    return std::unexpected(make_error_code(network_error::kDnsInitFailed));
                }
            }
            ++m_ares_count;
        }


        auto self = std::unique_ptr<resolver>{new resolver{executor, timeout}};

        ares_options options{};
        options.sock_state_cb = resolver::ares_socket_state_callback;
        options.sock_state_cb_data = self.get();

        auto channel_init_status = ares_init_options(&self->m_ares_channel, &options, ARES_OPT_SOCK_STATE_CB);
        if(channel_init_status != ARES_SUCCESS) {
            return std::unexpected(make_error_code(network_error::kDnsInitFailed));
        }

        return self;
    }

    resolver(const resolver &) = delete;
    resolver(resolver &&) = delete;
    resolver & operator=(const resolver &) noexcept = delete;
    resolver & operator=(resolver &&) noexcept = delete;

    ~resolver() {
        if(m_ares_channel != nullptr) {
            ares_destroy(m_ares_channel);
            m_ares_channel = nullptr;
        }

        {
            std::scoped_lock g{m_ares_mutex};
            --m_ares_count;
            if(m_ares_count == 0) {
                ares_library_cleanup();
            }
        }
    }

    
    silicon::scheduler::task<std::unique_ptr<result<executor_type>>> host_by_name(const network::hostname &hn) {
        silicon::coroutine::event resume_event{};
        auto result_ptr = std::make_unique<result<executor_type>>(m_executor, resume_event, 1);

        ares_addrinfo_hints hints{};
        hints.ai_family = AF_UNSPEC;

        ares_getaddrinfo(
                m_ares_channel,
                hn.data().data(),
                nullptr,
                &hints,
                ares_dns_callback,
                result_ptr.get()
        );


        co_await resume_event;
        co_return result_ptr;
    }

  private:

    resolver(std::unique_ptr<executor_type> &executor, std::chrono::milliseconds timeout)
        : m_executor(executor),
          m_timeout(timeout) {
    }


    std::unique_ptr<executor_type> &m_executor;


    std::chrono::milliseconds m_timeout{0};


    ares_channel m_ares_channel{nullptr};



    std::unordered_map<silicon::scheduler::fd_t, silicon::scheduler::poll_op> m_active_sockets{};

    silicon::scheduler::task<void> make_poll_task(silicon::scheduler::fd_t fd) {

        while(m_active_sockets.contains(fd)) {
            auto ops = m_active_sockets[fd];
            auto result = co_await m_executor->poll(fd, ops, m_timeout);
            switch(result) {
                case silicon::scheduler::poll_status::read:
                    ares_process_fd(m_ares_channel, fd, ARES_SOCKET_BAD);
                    break;
                case silicon::scheduler::poll_status::write:
                    ares_process_fd(m_ares_channel, ARES_SOCKET_BAD, fd);
                    break;

                case silicon::scheduler::poll_status::timeout:
                    ares_process_fd(m_ares_channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
                    break;
                case silicon::scheduler::poll_status::closed:

                    m_active_sockets.erase(fd);
                    break;
                case silicon::scheduler::poll_status::error:

                    m_active_sockets.erase(fd);
                    break;
                case silicon::scheduler::poll_status::cancelled:
                    m_active_sockets.erase(fd);
                    break;
            }
        }

        co_return;
    }

    static void ares_socket_state_callback(void *data, ares_socket_t socket_fd, int readable, int writable) {
        resolver *self = static_cast<resolver *>(data);
        uint64_t ops{0};

        if(readable) {
            ops |= static_cast<uint64_t>(silicon::scheduler::poll_op::read);
        }
        if(writable) {
            ops |= static_cast<uint64_t>(silicon::scheduler::poll_op::write);
        }

        auto fd = static_cast<silicon::scheduler::fd_t>(socket_fd);
        auto poll_ops = static_cast<silicon::scheduler::poll_op>(ops);
        if(ops != 0) {
            auto [it, inserted] = self->m_active_sockets.insert_or_assign(fd, poll_ops);
            if(inserted) {
                self->m_executor->spawn_detached(self->make_poll_task(fd));
            }
        } else {
            self->m_active_sockets.erase(fd);
        }
    }

    static void ares_dns_callback(void *arg, int status, int , ares_addrinfo *addr_info) {
        auto &result = *static_cast<silicon::network::dns::result<executor_type> *>(arg);
        --result.m_pending_dns_requests;

        if(addr_info == nullptr || status != ARES_SUCCESS) {
            result.m_status = status::kError;
        } else {
            result.m_status = status::kComplete;

            for(ares_addrinfo_node *node = addr_info->nodes; node != nullptr; node = node->ai_next) {


                if(node->ai_family == AF_INET) {
                    sockaddr_in *sin = reinterpret_cast<sockaddr_in *>(node->ai_addr);
                    auto ip_addr = network::ip_address::from_binary(
                            std::span<const uint8_t>{
                                    reinterpret_cast<const uint8_t *>(&sin->sin_addr), network::ip_address::ipv4_len
                            },
                            static_cast<network::domain_t>(AF_INET)
                    );

                    if(ip_addr) {
                        result.m_ip_addresses.emplace_back(std::move(*ip_addr));
                    }
                } else if(node->ai_family == AF_INET6) {
                    sockaddr_in6 *sin6 = reinterpret_cast<sockaddr_in6 *>(node->ai_addr);
                    auto ip_addr = network::ip_address::from_binary(
                            std::span<const uint8_t>{
                                    reinterpret_cast<const uint8_t *>(&sin6->sin6_addr), network::ip_address::ipv6_len
                            },
                            static_cast<network::domain_t>(AF_INET6)
                    );

                    if(ip_addr) {
                        result.m_ip_addresses.emplace_back(std::move(*ip_addr));
                    }
                }
            }

            ares_freeaddrinfo(addr_info);
        }

        if(result.m_pending_dns_requests == 0) {
            result.m_resume.set(result.m_executor, silicon::coroutine::resume_order_policy::kLifo);
        }
    }
};

}
