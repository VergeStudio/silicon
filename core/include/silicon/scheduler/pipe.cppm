module;


#include <array>
#include <expected>
#include <memory>
#include <system_error>


#include <silicon/common.h>
export module silicon.scheduler:pipe;

import :fd;

export namespace silicon::scheduler {

class CORE_API pipe_t {
  public:
    explicit pipe_t();
    ~pipe_t();

    /// 管道是否成功建立（底层 `_pipe`/`::pipe` 调用成功且未处于无效状态）。
    /// 构造不再抛异常：失败时 `pipe_t` 处于无效状态，须由调用方显式检查。
    [[nodiscard]] bool is_valid() const noexcept;

    /// 工厂：创建事件管道，失败时返回 `unexpected(scheduler_error::kPipeCreateFailed)`。
    [[nodiscard]] static std::expected<pipe_t, std::error_code> create();

    pipe_t(const pipe_t &other);
    pipe_t(pipe_t &&other) noexcept;

    pipe_t & operator=(const pipe_t &other) ;
    pipe_t & operator=(pipe_t &&other) noexcept ;

    [[nodiscard]] long write(const void *, std::size_t) ;
    [[nodiscard]] long read(void *, std::size_t) ;

    [[nodiscard]] const fd_t & read_fd() const ;
    [[nodiscard]] const fd_t & write_fd() const ;

    void close() ;

  private:
    struct impl;
    std::unique_ptr<impl> m_p;
};

} // namespace silicon::scheduler
