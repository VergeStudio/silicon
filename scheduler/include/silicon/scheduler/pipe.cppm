module;


#include <array>
#include <memory>


export module silicon.scheduler:pipe;

import :fd;

export namespace silicon::coroutine {

class pipe_t {
  public:
    explicit pipe_t();
    ~pipe_t();

    pipe_t(const pipe_t &other);
    pipe_t(pipe_t &&other) noexcept;

    pipe_t & operator=(const pipe_t &other) ;
    pipe_t & operator=(pipe_t &&other) noexcept ;

    [[nodiscard]] long write(const void *bytes, std::size_t n) ;
    [[nodiscard]] long read(void *buffer, std::size_t n) ;

    [[nodiscard]] const fd_t & read_fd() const ;
    [[nodiscard]] const fd_t & write_fd() const ;

    void close() ;

  private:
    struct impl;
    std::unique_ptr<impl> m_p;
};

} // namespace silicon::coroutine
