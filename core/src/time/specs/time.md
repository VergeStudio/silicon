# Spec: silicon.time — 统一时间与定时器

## 职责
为 siliconcode 提供零外部依赖、单一可替换的**时间**与**定时器**能力，作为全工程统一的时间来源：
- 墙上时钟 / 时间戳（system_clock）
- 单调时钟 / 计时（steady_clock）
- 线程休眠（sleep）
- 逻辑定时器（timing_wheel）

其他模块**不得裸用** `std::chrono::steady_clock` / `std::chrono::system_clock` /
`std::this_thread::sleep_for` / C 时间函数（`std::time`、`<ctime>` 等），一律经由本模块；
如功能不足，应**扩展本模块**而非在调用处自行实现。

## 接口
- `system_clock`：墙上时钟。`now() -> std::chrono::system_clock::time_point`、
  `now_ms() -> int64_t`（Unix 毫秒时间戳）。用于时间戳、日期、日志时间。
- `steady_clock`：单调时钟。`now() -> std::chrono::steady_clock::time_point`、
  `now_ns()/now_ms() -> int64_t`（自单调纪元计数）。用于计时、超时、性能度量；
  **不**受系统时间调整影响，绝不能当墙上时间戳。
- `sleep_for(duration)`：阻塞当前线程指定时长（等价于 `std::this_thread::sleep_for`）。
- `timing_wheel` + `timer_handle`：纯逻辑单层时间轮定时器，由调用方 `advance()` 驱动，
  非线程安全、可单测；`timer_handle` 为不透明取消句柄。
- `clock_facade` / `clock_proxy` / `date_source`：时钟代理与 `YYYY-MM-DD` 日期来源
  （基于 `system_clock` + 可选时区偏移）。

## 与 scheduler 定时器边界
- `silicon::scheduler::time` 仅作为 `core/time` 的薄别名：
  `clock = silicon::time::steady_clock`、`time_point = silicon::time::steady_clock::time_point`。
- `silicon::scheduler::timer_handle` 是**事件循环专用的 OS 定时器描述符包装**
  （timerfd / threadpool-timer / iocp），与 IO 多路复用强耦合，须留在 scheduler；
  其单调时间基准统一取自 `core/time`。应用层定时请使用 `timing_wheel`。

## 平台依赖
纯标准库，无平台相关文件（date_source 的 `gmtime_r`/`gmtime` 平台差已在
`time_unix.cpp` / `time_win.cpp` 内隔离）。
