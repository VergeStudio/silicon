# Spec: silicon.cli — 命令行参数解析

## 职责
为 siliconcode CLI 提供轻量命令行参数解析：子命令、位置参数、命名标志（`--name value` / `--flag`）。

## 接口
- `i_parser`：解析 `argv`，返回 `std::expected<parse_result, std::error_code>`。
- `Parser`：默认实现，支持通过声明式 schema 做失败校验。

## 解析结果 `parse_result`
- `command()`：首个非 flag 位置参数（子命令），无则为空。
- `flags()`：`--key value` 或 `--flag → "true"` 的映射。
- `positional()`：其余位置参数。

## 失败语义（expected 返回）
`Parse()` 返回 `std::expected<parse_result, std::error_code>`；成功含值，失败返回
`silicon::cli::cli_error` 对应的 `std::error_code`：

| 错误 | 触发条件 |
| --- | --- |
| `kUnknownSubcommand` | 提供了子命令但不在已声明的子命令集合中 |
| `kUnknownOption`     | 遇到未声明的命名标志 |
| `kMissingArgument`   | 声明为「需值」的标志未携带值 |
| `kInvalidValue`      | 畸形 flag（裸 `-` 或 `--`，无有效名称）|

> **宽松原则**：仅对「已声明」的维度做校验。未调用 `AddSubcommand` / `AddFlag`
> 时对应维度不做限制，行为与通用收集式解析一致（不报错）。

## Schema API（Parser）
- `void AddSubcommand(std::string name)`：登记合法子命令。
- `void AddFlag(std::string name, bool requires_value = false)`：登记合法标志；
  `requires_value=true` 时该 flag 必须后接一个非 `-` 开头的值。

## 平台
纯标准库，无平台相关文件。
