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
// 为什么必须 `export` 且必须落在本分区：
//   * MSVC 不把「非导出实体」暴露给实现单元（实现单元即使写了
//     `import :poll_info_impl;` 也只能看到 :poll_info 中的前置声明），
//     故此处必须用 export；
//   * 本分区的主模块接口单元 scheduler.cppm 只做普通 `import :poll_info_impl;`
//     而非 `export import`，因此该定义不会随主接口重新导出，模块外部消费者
//     仍然只拿到不完整类型（PIMPL 封装不变）；
//   * 它需要是命名空间作用域类型而非 poll_info 的嵌套类：MSVC 无法把外围类所在
//     模块单元之外给出的嵌套类定义写进 IFC（详见 poll_info.cppm 中的说明）。
//
// 因此，凡要在 purview 内使用完整实现的本模块实现单元，都必须显式写
// `import :poll_info_impl;`——只 import 主模块接口是不够的。
//
// 本分区的 purview 内**不要** #include 任何标准头：所需的 <coroutine> <map>
// <optional> 等已在上方全局模块片段中包含，purview 内的 #include 会与 IFC 携带的
// std 声明重复附着而触发 C2953。
export namespace silicon::scheduler {

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
