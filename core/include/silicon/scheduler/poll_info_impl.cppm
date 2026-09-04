module;


#include <atomic>
#include <coroutine>
#include <map>
#include <memory>
#include <optional>
#include <utility>


export module silicon.scheduler:poll_info_impl;

import :fd;
import :poll;
import :time;
import :poll_info;

// 基础 I/O 类型已随调度原语迁入本模块（命名空间仍为 silicon::coroutine），
// 此处仅在本单元内引入简化书写，不 export。
namespace silicon::scheduler {
using silicon::coroutine::fd_t;
using silicon::coroutine::poll_op;
using silicon::coroutine::poll_op_readable;
using silicon::coroutine::poll_op_writeable;
using silicon::coroutine::poll_status;
using silicon::coroutine::poll_stop_token;
using silicon::coroutine::time_point;
} // namespace silicon::scheduler

// poll_info 的 PIMPL 实现类型（poll_info::impl 即本类型的别名）。
//
// 定义刻意**不 export**，与 :poll_info 分区中 `struct poll_info_impl;` 的非导出
// 前置声明保持一致：实体只有模块链接，属模块内部实现类型，不进入模块对外接口。
// 需要完整定义的本模块单元（接口分区与实现单元）显式写 `import :poll_info_impl;`
// 即可使用；主模块接口单元 scheduler.cppm 亦只做普通 import 而非 export import，
// 因此 `import silicon.scheduler;` 的消费方始终只拿到不完整类型（PIMPL 封装不变）。
//
// 类型必须是命名空间作用域而非 poll_info 的嵌套类：MSVC 无法把外围类所在模块单元
// 之外给出的嵌套类定义写进 IFC（详见 poll_info.cppm 中的说明）。
//
// 本分区的 purview 内**不要** #include 任何标准头：所需的 <coroutine> <map>
// <optional> 等已在上方全局模块片段中包含，purview 内的 #include 会与 IFC 携带的
// std 声明重复附着而触发 C2953。
namespace silicon::scheduler {

/// Implementation state of a poll operation: target descriptor, requested
/// operation, the paired timeout's position in `io_scheduler`'s timed events
/// map, the awaiting coroutine, the observed result and the cancellation hook.
struct poll_info_impl {
    /// 被轮询的文件描述符 / 句柄。
    fd_t m_fd{-1};
    /// 本轮关注的事件（可读 / 可写 / 读写）。
    poll_op m_op{};
    /// 配对超时事件在 io_scheduler 定时事件表中的位置，未挂定时则为空。
    std::optional<poll_info::timed_events::iterator> m_timer_pos{std::nullopt};
    /// 挂起等待本轮询结果的协程句柄。
    std::coroutine_handle<> m_awaiting_coroutine;
    /// 事件或超时二者中先被处理者写入的最终结果。
    poll_status m_poll_status{poll_status::error};
    /// 同一批事件 / 超时只承认首个：先到者置 true，后续同批事件被丢弃。
    bool m_processed{false};
    /// 可选的取消触发器（stop_token 侧），非空时说明本次轮询可被外部取消。
    std::optional<poll_stop_token> m_cancel_trigger{std::nullopt};
};

} // namespace silicon::scheduler
