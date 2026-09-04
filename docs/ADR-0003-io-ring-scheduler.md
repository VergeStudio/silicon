# ADR-0003：io_ring 接入 io_scheduler —— completion I/O 第二层（Option A）

- 状态：**已采纳（Accepted）**
- 日期：2026-09-04
- 范围：silicon 异步运行时 / scheduler completion I/O 引擎
- 关联：`core/include/silicon/scheduler/{io_scheduler,io_notifier,io_ring,io_op,error,scheduler}.cppm`、
  `core/src/scheduler/io_scheduler_completion.cpp`、`core/xmake.lua`

## 背景（Context）

`io_scheduler` 的事件驱动基于 **readiness 模型**（`io_notifier`：epoll / kqueue / IOCP 的
socket/pipe/timer 就绪事件 + `poll_info` 单次状态机）。它无法服务**带偏移的常规文件读写**：

- 硬约束：普通文件在 epoll/kqueue/WSAPoll 下不可 readiness 轮询（epoll `EPERM`；
  WSAPoll `WSAENOTSOCK`）→ 文件 async I/O 只有 completion 引擎或线程池阻塞两条路，
  "文件回退 readiness" 不成立。
- 仓库已有一层 `io_ring`（Linux=io_uring / Windows=I/O Ring）的**平台封装**（接口
  `io_ring.cppm` + 后端 `io_ring_uring.cpp` / `io_ring_ioring.cpp`），可提交带偏移
  读/写并收割 CQE，但尚未接入 `io_scheduler` 的事件驱动模型。
- 现状缺陷：`core/xmake.lua` 在任意平台开 `--io_ring=y` 即定义
  `SILICON_FEATURE_IO_RING`，而 mac/BSD 无后端实现单元 → io_ring 构造符号悬空、
  链接失败。

目标：让 `io_scheduler` 以**可选引擎**方式支持 `read_at/write_at(fd,buf,len,offset)`
（`task<result<int64_t>>`），同时默认构建与现状字节级等价、零回归。

## 决策（Decision）

**Option A —— 双引擎组合（composition）+ 专用 completion worker 线程 + 运行期探测回退。**

- **D1 双引擎组合**：不统一 backend 虚接口。readiness（何时可读写）与 completion
  （做了多少字节）是两套不可互相替代的状态机。`io_scheduler` 保持单线程事件驱动
  不变；io_ring 的 SQ/CQ 归一个**专用 completion worker 线程**（惰性创建）独占。
  事件驱动线程负责 readiness + drain completion；两条引擎经"能力查询 + 显式路由"耦合，
  无继承、无虚函数。
- **D2 路由矩阵**：按 **API 形态**定路由而非运行期猜 fd。既有 `poll()` 只服务
  readiness 就绪对象；新增 `read_at/write_at` 只服务**常规文件**（`fstat+S_ISREG` /
  `GetFileType==FILE_TYPE_DISK` 预检），非常规 fd → `kNotRegularFile`；后端不可用 →
  立即 `kNoCompletionBackend`（不挂起、不阻塞）。
- **D3 xmake 开关**：扩展现有 `io_ring` 布尔选项并**修正宏作用域**——
  `SILICON_FEATURE_IO_RING` 仅在 linux/windows 定义。`options` 追加
  `io_completion_policy{disabled,enabled,auto_}` 与 `io_ring`(io_ring_config) 字段
  （追加于末尾，designated-initializer 兼容；默认值由编译宏推导：宏在 → `auto_`，
  否则 `disabled`）。
- **D4 运行期探测 + 显式回退**：首次 `read_at/write_at` 以 `std::once_flag` 惰性
  一次性探测（policy/能力检查 → io_ring 构造 → `is_valid/active_backend/supports` →
  建 engine + 内部唤醒通道 → 启 worker）；任一失败记 backend=none 终态，后续直接
  `kNoCompletionBackend`。状态迁移：未初始化 → 初始化中(once) → 可用|不可用。
  错误解码：CQE `result>0`=传输字节数；Linux `<0`=`-errno`→`generic_category`；
  Windows 负 HRESULT→`system_category`（F2 待实证）。

内部实现要点（Actor 模型）：

- 生产者（read_at/write_at 协程）：文件预检 → 探测/取引擎 → `m_size.fetch_add` →
  栈上（协程帧内）`io_op` 入「待提交」MPSC 队列 → `co_await op`。
- worker：pop「待提交」→ `submit_read/write` → `submit()` → `wait_completion(节拍)`
  → CQE `user_data==io_op*` → 错误解码 → push「已完成」→ 唤醒驱动。
- 驱动：`process_events_execute` 的哨兵分支 `handle_ptr==m_completion_ptr →
  drain_ring_completions()`：排空唤醒通道 → `pop_all` 完成队列 → 反转 → 自旋等
  `m_awaiting_coroutine` 可见 → 排入 `m_handles_to_resume` 统一恢复。
- 唤醒通道（设计 R4）：POSIX=内部 completion pipe（worker 写 1 字节，epoll/kqueue
  轮询，以 impl 内真实 `poll_info` 哨兵为 udata 注册 keep=true）；Windows=
  `io_notifier::post` = `PostQueuedCompletionStatus`（key=哨兵）。**Windows 禁接 CRT
  schedule pipe**——IOCP 等待语义无法被普通管道写可靠打断。
- 生命周期：engine 归 `io_scheduler::impl` 的两个 opaque 槽（`void*`，平台类型不出
  现在 cppm）；析构顺序 shutdown() → join → `destroy_completion_engine()`（停 worker
  → 排空防御性完结 → join → 关 ring）→ 关既有管道。v1 无超时/取消 → 无"CQE 迟到
  写已释放缓冲"的僵尸 op（契约：操作期间 fd 不得被其他线程关闭、缓冲存活到 task 完成）。

## 理由（Rationale）

1. **readiness 与 completion 领域正交**：前者回答"何时可读写"（poll_info→poll_status，
   单次就绪完成），后者回答"做了多少字节"（fd+缓冲+偏移→字节数）。压进统一虚接口会
   破坏 `task<poll_status>` 返回类型与模块导出面，在 MSVC + C++20 modules 下放大风险。
2. **Actor 单写者保证 io_ring 免改造**：io_ring 方法只在专用 worker 线程触碰 → 无需
   线程安全改造；事件驱动线程保持既有严格单线程状态机不变。
3. **按 API 形态路由跨平台一致**：Linux io_uring 与 Windows IORing 提交面逐字节一致
   （v1 不让 socket 走 completion，跨平台统一）；预检 + 降级路径保证 create() 依旧成功、
   调度器纯 readiness 可运行，默认构建字节级等价。
4. **显式降级优于失败**：无后端立即 `kNoCompletionBackend`（不挂起、不阻塞）符合
   "能力查询 + 显式路由"耦合，把"平台不可用"从调用方的行为错误里剔除。
5. **运行期探测**优于"只按构建期宏判断"：同一二进制可同时表达"编译了后端但内核/系统
   不支持"（如 io_uring 内核过旧、Windows 版本禁用 I/O Ring），状态以 `completion_backend()`
   终态对外暴露。

## 后果（Consequences）

- **正面**：常规文件带偏移 async I/O 进入 io_scheduler；mac/BSD 默认构建与既有
  readiness 行为完全等价；`--io_ring=y` 在无后端平台不再链接失败。
- **负面**：completion 路径是独立引擎 + worker 线程，比单线程 readiness 多一次跨线程
  队列/唤醒开销；io_uring 真路径只能在 Linux/Windows CI 以 `--io_ring=y` 验证
  （mac 仅验证默认契约，R1）。
- **可回退**：`read_at/write_at` 为新追加 API、错误码追加于枚举末尾、`options` 字段
  只追加末尾，不触碰既有 ABI/行为；关闭 `io_ring` 即回到纯 readiness，决策可逆。

## 备选方案（Alternatives Considered）

- **Option B（配置期字符串选项）**：`--scheduler_io=auto|completion|readiness` +
  config 期 raiseerror 校验 + 额外宏 `SILICON_SCHEDULER_IO_COMPLETION`。否决：readiness
  后端（epoll/kqueue/iocp）本就是平台宏 + 文件守卫编译期唯一决定，不伪装成字符串枚举；
  Option A 以"布尔 io_ring + 运行期探测"表达同样意图且改动面更小。
- **统一 backend 虚接口（readiness+completion 同构）**：否决（理由 1，领域正交）。
- **io_scheduler 单线程直接驱动 io_ring（不建 worker）**：与"严格单线程事件驱动 +
  io_ring 阻塞 wait"冲突，且 Linux eventfd 直连与 Windows 无等价物会形成两条路径；
  v1 不采用，eventfd 直连记为 v1.1 优化。

## 参考

- `core/include/silicon/scheduler/io_ring.cppm`（平台无关 completion I/O 环）
- `core/include/silicon/scheduler/io_scheduler.cppm`（options 新字段 / read_at·write_at /
  completion_backend / opaque 槽）
- `core/src/scheduler/io_scheduler_completion.cpp`（completion 引擎 / worker / drain）
- `core/include/silicon/scheduler/io_notifier.cppm`（`post` 声明）
- `core/src/scheduler/io_notifier_iocp.cpp`（`post`=PostQueuedCompletionStatus）
- `.workbuddy/memory/io-ring-layer2-design-2026-09-04.md`（设计快照）
