module;

#include <array>
#include <expected>
#include <memory>
#include <system_error>

#include <silicon/common.h>
export module silicon.scheduler:pipe;


export namespace silicon::scheduler {

class SILICON_CORE_API pipe_t {
  public:
    explicit pipe_t();
    ~pipe_t();

    [[nodiscard]] bool is_valid() const noexcept;

    [[nodiscard]] static std::expected<pipe_t, std::error_code> create();

    pipe_t(const pipe_t &other);
    pipe_t(pipe_t &&other) noexcept;

    pipe_t & operator=(const pipe_t &other) ;
    pipe_t & operator=(pipe_t &&other) noexcept ;

    [[nodiscard]] long write(const void *, std::size_t) ;
    [[nodiscard]] long read(void *, std::size_t) ;

    [[nodiscard]] const int & read_fd() const ;
    [[nodiscard]] const int & write_fd() const ;

    void close() ;

  private:
    struct impl;
    std::unique_ptr<impl> m_p;
};

}

// 非导出：pipe_t 的 pimpl 结构（平台无关），供 pipe.cpp 与各平台实现单元共享。
namespace silicon::scheduler {

struct pipe_t::impl {
    std::array<int, 2> m_fds{-1};
};

}
