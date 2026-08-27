# ADR-0002：调度器架构 —— silicon::scheduler/task 与 stdexec P2300 的关系

- 状态：**已采纳（Accepted）**
- 日期：2026-08-27
- 范围：silicon 异步运行时 / 任务模型
- 关联：`scheduler/include/silicon/scheduler/*`、`core/include/silicon/core/proxy`

## 背景（Context）

silicon 已有一套自有调度器与任务抽象：

- `silicon::scheduler::task<T>`：libcoro/imperative 风格的协程任务类型，与 IO 异步栈域耦合
  （`io_scheduler` 用 `poll`/`WSAPoll` 驱动，`schedule_after`/`yield_for`/`process_events` 做定时与事件），
  支持 `when_any` 超时/竞速取消（见 `io_scheduler.cppm` 的 `schedule(task, timeout)` 重载，返回
  `expected<return_type, timeout_status>`）。
- `silicon::scheduler::expected<T, E>`：统一 `std::expected<T, std::error_code>` 别名（`scheduler/expected.cppm`），
  错误以值传递，不抛异常。
- `scheduler_facade`：基于 Microsoft proxy v4 的类型擦除调度器契约
  （`spawn_detached`/`spawn_joinable`/`resume`/`shutdown`/`is_shutdown`/`size`/`empty`），
  任意具体调度器（thread_pool / io_scheduler / inline_scheduler / parallel_scheduler / run_loop）鸭子类型满足即可，无需继承。

同时，C++ 社区存在 **stdexec（P2300）**：declarative sender/receiver 图，配 `stop_token` +
三通道完成（value / error / stopped），有 `std::execution` 与 `exec::task` 等实现。

问题：是否应以 stdexec/P2300 替换 `silicon::scheduler::task`，作为统一的异步计算模型？

## 决策（Decision）

**保留 `silicon::scheduler` / `silicon::task` 作为 silicon 的异步运行时；不采用 stdexec/P2300 替换之。**
`std::execution` / `exec::task` **不可替代** `silicon::task`。两者作用在不同层次，是互补而非替代关系。

接口层面：

- `task_scheduler` 统一契约维持现有 **proxy 门面**（`scheduler_facade` + `scheduler_proxy`/`scheduler_view` +
  `make_scheduler<T>()`），不引入 sender/receiver 后端。
- `task<T>` 仍为协程任务类型；`expected<T, E>` + `when_any` 超时/竞速取消保留。
- 若未来需要，可**增量**提供一个 `scheduler → std::execution::scheduler` 适配器（把 silicon 调度器暴露为
  sender 的执行上下文），作为可选互操作层，而非重写任务模型。

## 理由（Rationale）

1. **领域正交**：`silicon::task` 是**协程 + IO 异步栈域**（可 await、恢复裸 `coroutine_handle`、与
   `io_scheduler` 的 poll 循环、超时、取消深度集成）；stdexec 是**纯计算调度**的 declarative 组合模型
   （lazy sender 图 + 算法定制点）。前者管"异步控制流与 IO"，后者管"计算图的惰性组合"，职责不同，
   不存在可替换性。
2. **IO 集成已具备**：silicon 已有 `io_scheduler`（poll/WSAPoll、定时、事件）+ `when_any` 超时/竞速取消
   + `expected` 错误处理。stdexec 本身不提供这些 IO/超时能力，采用它会丢失现有集成。
3. **编译器与模块成熟度**：P2300 库（sender/receiver/algorithm 大量定制点）在 **MSVC + C++20 modules**
   下支持尚不成熟，引入会带来 ABI/编译期耦合风险，与 silicon 当前"MSVC 原生 std 模块 + 单 core.dll"
   的稳定构建目标冲突。
4. **类型擦除已就位**：调度器边界已用 proxy v4 门面统一，任意实现鸭子类型满足即可驱动 `task_group`/上层，
   无需 stdexec 即可获得"统一调度接口"。
5. **成本收益**：替换意味着重写任务模型、awaitable integration、全部既有调用点，收益仅"算法组合糖"，
   且该功能可后续以 sender 适配器低成本补回。

## 后果（Consequences）

- **正面**：异步运行时稳定、构建可控；IO/超时/取消能力保持；调度器接口统一于 proxy 门面，可插拔。
- **负面**：放弃 stdexec 的 sender 组合算法生态；若未来需要复杂计算图组合，需自建或接适配器。
- **可回退**：若 P2300 在 MSVC/modules 下成熟，可在不破坏 `task<T>` 的前提下加 sender 适配器，决策可逆。

## 备选方案（Alternatives Considered）

- **A. 全量迁移到 stdexec/P2300**：否决（理由 1–4，成本高、丢失 IO 集成、MSVC/modules 不成熟）。
- **B. 双轨：task + sender 并存**：暂不采用；仅在确有计算图组合需求时，以 sender 适配器（方案 2 增量）引入，
  保持 `task<T>` 为主模型。
- **C. 现状（维持 silicon::task）**：采纳。

## 参考

- `scheduler/include/silicon/scheduler/facade.cppm`（`scheduler_facade` + proxy 门面）
- `scheduler/include/silicon/scheduler/io_scheduler.cppm`（`when_any` 超时、`expected`）
- `scheduler/include/silicon/scheduler/expected.cppm`（统一 `expected` 别名）
- P2300 (stdexec)：sender/receiver + `stop_token` + 三通道完成（value/error/stopped）
