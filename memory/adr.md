# 4. 关键架构决策（详见 docs/ADR-0001~0003，均已采纳落地）

- **proxy v4 类型擦除**（ADR-0001）：vendor 微软 proxy v4（`proxy_macros.h`）；plugin/scheduler/fs/di/http/logger/network/ai.llm/cli 等 facade 均以 `proxy<Facade>` 暴露非侵入式接口契约，规避跨 DLL/ABI 边界的虚表布局耦合。
- **scheduler/task 自研**（ADR-0002）：不走 stdexec P2300；自有 `scheduler::task<T>`（libcoro 风格协程任务，与 io_scheduler 异步栈域耦合，`when_any` 超时/竞速取消）+ `expected<T,E>`（`std::expected<T,std::error_code>` 别名，错误以值传递不抛异常）+ `scheduler_facade`（proxy 类型擦除调度契约：spawn_detached/spawn_joinable/resume/shutdown/size/empty；thread_pool/io_scheduler/inline_scheduler/parallel_scheduler/run_loop 鸭子类型满足）。
- **completion I/O 分层**（ADR-0003）：readiness 层 = `io_notifier`（epoll/kqueue/IOCP 就绪事件 + `poll_info` 状态机），只能服务 socket/pipe/timer；**常规文件不可 readiness 轮询**（epoll `EPERM` / WSAPoll `WSAENOTSOCK`），只能走 completion 引擎或线程池。第二层 = `io_ring`（Linux io_uring / Windows I/O Ring，支持带偏移 `read_at`/`write_at` + CQE 收割），已接入 io_scheduler。
