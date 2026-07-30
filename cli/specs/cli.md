# Spec: silicon.cli — 命令行参数解析

## 职责
为 siliconcode CLI 提供轻量命令行参数解析：子命令、位置参数、命名标志（`--name value` / `--flag`）。

## 接口
- `ICliParser`：解析 `argv`，返回解析结果（标志值、子命令、剩余参数）。
- `CliParser`：默认实现。

## 平台
纯标准库，无平台相关文件。
