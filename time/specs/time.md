# Spec: silicon.time — 时钟与时区

## 职责
为 siliconcode 提供零外部依赖的时钟、时区、日期 Context Source 能力。支撑 System Context 的 DateSource、Epoch 时间戳、以及会话运行时的时间相关操作。

## 接口
- `IClock`：`now() -> std::chrono::system_clock::time_point` 和 `now_system_ms() -> int64_t`（Unix 毫秒时间戳）。
- `IDateSource`：`current_date() -> std::string`（ISO 8601 日期字符串 `YYYY-MM-DD`），基于 IClock + 可选的时区偏移。
- `SystemClock`：IClock 默认实现，包装 `std::chrono::system_clock::now()`。

## 平台依赖
纯标准库，无平台相关文件。
