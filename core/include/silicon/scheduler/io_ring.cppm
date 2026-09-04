// Interface partition silicon.scheduler:io_ring
//
// 平台无关的 completion I/O 环：调用方提交操作、收割完成项，读写由内核直接
// 完成（Linux=io_uring，Windows=I/O Ring）。与 :io_notifier 的 readiness 模型
// 互补而非替代——io_notifier 负责"fd 何时可读写"，io_ring 负责"读写已经做完"。
//
// 每个平台只暴露**同一个** `io_ring` 类，平台专属状态（struct io_uring /
// HIORING / IORING_*）隐藏在私有的嵌套 `struct impl` 之中。`impl` 的实体定义
// 位于各平台实现单元（io_ring_uring.cpp / io_ring_ioring.cpp），因此平台专属
// 类型**绝不出现在本接口单元**，任何触及 impl 成员的方法都必须在各自平台的
// .cpp 中**非内联**实现。
//
// 后端不可用时（未开启 SILICON_FEATURE_IO_RING、内核过旧、Windows 版本禁用
// I/O Ring）is_valid() 返回 false，消费方应回退到 :io_notifier（IOCP / epoll
// / kqueue）。注意：本类**不重新实现 IOCP**，回退发生在消费方一侧。

module;

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include <silicon/common.h>
export module silicon.scheduler:io_ring;

import :fd;

// 基础 I/O 句柄类型已在 :fd 中定义（命名空间仍为 silicon::coroutine），此处仅
// 在本单元内引入简化书写，不 export。
namespace silicon::scheduler {
using silicon::coroutine::fd_t;
} // namespace silicon::scheduler

export namespace silicon::scheduler {

/// io_ring 的构造配置。
///
/// 置于命名空间作用域而非 io_ring 内部：带默认成员初始化器的嵌套类不能在外围类
/// 定义体内用作默认实参（clang: default member initializer needed within
/// definition of enclosing class）。
struct io_ring_config {
    /// SQ/CQ 环深度，实现会向上取整到 2 的幂。
    std::uint32_t queue_depth{256};
    /// Linux 专用：启用 SQPOLL 内核轮询线程（内核 5.11+，且需足够权限）。
    /// Windows 的 I/O Ring 无对应能力，忽略本字段。
    bool sq_poll{false};
};

/// 平台无关的 completion I/O 环。
class CORE_API io_ring {
    struct impl;
    std::unique_ptr<impl> m_p;

  public:
    /// 环支持的操作类别。Windows 的 I/O Ring 只覆盖文件读写与取消，
    /// 套接字 accept / connect 不在其中，须走 io_notifier 的 IOCP 路径。
    enum class op {
        read,
        write,
        cancel
    };

    /// 实际生效的后端。none 表示本对象不可用，消费方应回退 io_notifier。
    enum class backend {
        none,
        io_uring,
        windows_io_ring
    };

    /// 单个完成项。
    struct completion {
        /// 提交时携带的关联值，原样回填。
        std::uint64_t user_data{0};
        /// 成功为传输字节数；失败为负 errno（Linux）或负 HRESULT/Win32（Windows）。
        std::int32_t result{0};
        /// 平台相关完成标志。
        std::uint32_t flags{0};
    };

    explicit io_ring(io_ring_config = {});
    ~io_ring();

    // 不可拷贝也不可移动：后端持有内核对象，移动会让已提交的 SQE 指向失效状态。
    io_ring(const io_ring &) = delete;
    io_ring(io_ring &&) = delete;
    io_ring & operator=(const io_ring &) = delete;
    io_ring & operator=(io_ring &&) = delete;

    /// 底层环是否成功建立。构造不抛异常：失败时处于无效状态，须由调用方显式检查。
    [[nodiscard]] bool is_valid() const noexcept;

    /// 实际生效的后端。
    [[nodiscard]] backend active_backend() const noexcept;

    /// 当前后端是否支持给定操作。无效对象一律返回 false。
    [[nodiscard]] bool supports(op) const noexcept;

    /// 提交一次读。offset 为文件偏移，直传内核；管道、套接字等不可寻址对象传 0。
    /// @return false 表示对象无效或 SQ 已满，调用方应先 submit() 再重试。
    bool submit_read(fd_t, void *, std::uint32_t, std::uint64_t, std::uint64_t) ;

    /// 提交一次写，参数语义同 submit_read。
    bool submit_write(fd_t, const void *, std::uint32_t, std::uint64_t, std::uint64_t) ;

    /// 请求取消此前以 target_user_data 提交的操作；
    /// 取消动作本身的完成项以 user_data 标识。
    bool submit_cancel(std::uint64_t, std::uint64_t) ;

    /// 将已排入 SQ 的条目一次性推入内核，返回实际提交条数。
    std::uint32_t submit() ;

    /// 阻塞等待至多一个完成项；超时或对象无效返回 nullopt。
    [[nodiscard]] std::optional<completion> wait_completion(std::chrono::milliseconds) ;

    /// 非阻塞收割一个已就绪完成项；无就绪项返回 nullopt。
    [[nodiscard]] std::optional<completion> peek_completion() ;
};

} // namespace silicon::scheduler
