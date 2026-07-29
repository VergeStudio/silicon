# Spec: silicon.fs — 文件系统抽象

## 职责
提供跨平台、平台无关的文件系统统一接口 `IFileSystem`，对外隐藏 Windows / POSIX 差异。用于 managed output 目录、AGENTS.md 发现、配置加载等。

## 统一接口 `IFileSystem`
- `read(path) -> Result<std::string, FsError>`：文本读取，UTF-8。
- `write(path, content) -> Result<void, FsError>`：文本写入（平台相关：Windows 归一化为 CRLF，POSIX 保持 LF）。
- `read_binary(path) -> Result<std::vector<std::byte>, FsError>`：二进制读取。
- `write_binary(path, data) -> Result<void, FsError>`：二进制写入。
- `exists(path) -> bool`
- `list_dir(path) -> Result<std::vector<std::string>, FsError>`
- `create_directories(path) -> bool`

## 分平台实现（编译期选用）
实现经 `fs.cppm` 按 `SILICON_PLATFORM_*` 宏（`顶层 xmake.lua` 定义）`#include` 进 `silicon.fs` 模块：
- `Win32FileSystem`（`include/silicon/fs/fs_win.hpp`）：文本写入归一化为 CRLF；目录创建使用系统默认权限。
- `PosixFileSystem`（`include/silicon/fs/fs_posix.hpp`）：文本保持 LF；目录创建后显式设置 0755 权限。
- 二者均继承共享基类 `FileSystemBase`（`include/silicon/fs/fs_common.hpp`，基于 `std::filesystem`），平台差异通过 `normalize_text()` 钩子与 `create_directories()` 重写下放。

## 对外入口
- `create_file_system() -> std::unique_ptr<IFileSystem>`：工厂，返回当前平台的实现，供调用方以接口形式使用，无需感知具体平台类型。
- 错误以 `Result<T, FsError>` 表达（fs 模块自有 bool 风格 `Result`，非异常）。

## 平台
`fs_win.hpp` / `fs_posix.hpp` 存放平台相关实现；共享逻辑在 `fs_common.hpp`。仅编译期按平台选择，无运行时分派、无独立平台翻译单元。
