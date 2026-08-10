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

    auto operator=(const pipe_t &other) -> pipe_t &;
    auto operator=(pipe_t &&other) noexcept -> pipe_t &;

    [[nodiscard]] auto write(const void *bytes, std::size_t n) -> long;
    [[nodiscard]] auto read(void *buffer, std::size_t n) -> long;

    [[nodiscard]] auto read_fd() const -> const fd_t &;
    [[nodiscard]] auto write_fd() const -> const fd_t &;

    auto close() -> void;

  private:
    struct Impl;
    std::unique_ptr<Impl> m_p;
};

} // namespace silicon::coroutine
