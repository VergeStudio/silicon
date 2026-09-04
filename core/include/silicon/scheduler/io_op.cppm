module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <system_error>
#include <utility>

export module silicon.scheduler:io_op;

import :fd;

// completion I/O 操作（io_ring 接入 io_scheduler 第二层）的内部状态类型。
//
// 定义刻意**不 export**，镜像 :poll_info_impl 分区的非导出模式：实体只有模块
// 链接，属模块内部实现类型，不进入模块对外接口。主模块接口单元 scheduler.cppm
// 只做普通 import 而非 export import，因此 `import silicon.scheduler;` 的消费方
// 永远看不到本类型。
//
// 与 poll_info 的 readiness 状态机互补：poll_info 描述"fd 何时可读写"，io_op
// 描述"带偏移的常规文件读写已由内核完成多少"。io_op 直接作为 MPSC 侵入式链表
// 节点（m_next）与 io_ring 的 user_data（== this）使用，声明周期归属挂起它的
// 协程帧（同 poll_info 局部变量模式），worker/驱动只在其入队到出队期间触碰。
//
// 本分区的 purview 内不 #include 任何标准头：所需 <coroutine> <cstdint>
// <expected> <system_error> 已在上方全局模块片段中包含。
namespace silicon::scheduler {

/// completion I/O 操作状态：目标 fd、缓冲/长度/偏移、挂起协程、完成结果与
/// 侵入链指针。非导出类型，方法定义位于 core/src/scheduler/io_op.cpp。
struct io_op {
    /// 目标常规文件的描述符 / CRT fd。
    silicon::coroutine::fd_t m_fd{-1};
    /// 读入 / 写出缓冲（写路径由 io_scheduler::write_at 以 const_cast 存入）。
    void *m_buffer{nullptr};
    /// 请求字节数（≤ 4GiB）。
    std::uint32_t m_length{0};
    /// 文件偏移，直传内核。
    std::uint64_t m_offset{0};
    /// true=write_at，false=read_at。
    bool m_is_write{false};
    /// 挂起等待本次操作完成的协程句柄。
    std::coroutine_handle<> m_awaiting_coroutine{nullptr};
    /// 同一完成项只承认首个消费者：驱动 drain 置位，防止同批重复恢复。
    bool m_processed{false};
    /// 成功传输字节数（result>0 时有效）。
    std::int64_t m_transfer{0};
    /// 失败错误（operator bool 为 true 时有效）。
    std::error_code m_error{};
    /// MPSC 侵入式单向链表节点。
    io_op *m_next{nullptr};

    /// 记录一次成功完成（字节数）。
    void complete(std::int64_t) noexcept ;
    /// 记录一次失败完成。
    void complete_error(std::error_code) noexcept ;
    /// 组装协程恢复时的返回结果。
    [[nodiscard]] std::expected<std::int64_t, std::error_code> result() const noexcept ;

    /// co_await io_op 使用的 awaiter：await_suspend 保存挂起协程句柄。
    struct io_awaiter {
        explicit io_awaiter(io_op &op) noexcept: m_op(op) {}
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> awaiting) noexcept ;
        std::expected<std::int64_t, std::error_code> await_resume() noexcept ;

        io_op &m_op;
    };

    io_awaiter operator co_await() noexcept { return io_awaiter{*this}; }
};

} // namespace silicon::scheduler
