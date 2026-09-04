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

// poll_info 的 PIMPL 实现类型（poll_info::impl 即本类型的别名）。
//
// 为什么这里必须 `export`：
//   MSVC 不会把分区里的**非导出**实体写进该分区的 IFC——实现单元即使显式写了
//   `import :poll_info_impl;`，也只能看到 :poll_info 分区中的前置声明，访问
//   `m_p->m_fd` 之类成员时即报 C2027「使用了未定义类型」。曾据此改为非 export
//   （理由是"与 :poll_info 中非导出前置声明保持一致"），结果整个 scheduler 模块
//   编译失败，故此处必须 export。
//
//   export 只影响模块内可见性，**不会**破坏 PIMPL 封装：主模块接口单元
//   scheduler.cppm 对本分区是普通 `import :poll_info_impl;` 而非 `export import`，
//   因此 `import silicon.scheduler;` 的外部消费方始终只拿到不完整类型。
//
// 类型必须是命名空间作用域而非 poll_info 的嵌套类：MSVC 无法把外围类所在模块单元
// 之外给出的嵌套类定义写进 IFC（详见 poll_info.cppm 中的说明）。
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
