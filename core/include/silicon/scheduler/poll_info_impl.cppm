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

// NOTE: poll_info::impl 的完整定义已迁至文本头 poll_info_impl.h（MSVC 对
// 导出分区内嵌套类 out-of-line 定义对实现单元不可见，见 poll_info_impl.h
// 注释），由各实现单元在 purview 文本包含。
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

