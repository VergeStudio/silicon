module;

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include <silicon/common.h>
export module silicon.scheduler:io_ring;


export namespace silicon::scheduler {

struct io_ring_config {

    std::uint32_t queue_depth{256};

    bool sq_poll{false};
};

class SILICON_CORE_API io_ring {
  private:
    struct impl;
    std::unique_ptr<impl> m_p;

  public:

    enum class op {
        read,
        write,
        cancel
    };

    enum class backend {
        none,
        io_uring,
        windows_io_ring
    };

    struct completion {

        std::uint64_t user_data{0};

        std::int32_t result{0};

        std::uint32_t flags{0};
    };

    explicit io_ring(io_ring_config = {});
    ~io_ring();

    io_ring(const io_ring &) = delete;
    io_ring(io_ring &&) = delete;
    io_ring & operator=(const io_ring &) = delete;
    io_ring & operator=(io_ring &&) = delete;

    [[nodiscard]] bool is_valid() const noexcept;

    [[nodiscard]] backend active_backend() const noexcept;

    [[nodiscard]] bool supports(op) const noexcept;

    bool submit_read(int, void *, std::uint32_t, std::uint64_t, std::uint64_t) ;

    bool submit_write(int, const void *, std::uint32_t, std::uint64_t, std::uint64_t) ;

    bool submit_cancel(std::uint64_t, std::uint64_t) ;

    std::uint32_t submit() ;

    [[nodiscard]] std::optional<completion> wait_completion(std::chrono::milliseconds) ;

    [[nodiscard]] std::optional<completion> peek_completion() ;
};

}
