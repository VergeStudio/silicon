# Spec: silicon.fs — 文件系统抽象

## 职责
提供跨平台、平台无关的文件系统统一接口 `IFileSystem`，对外隐藏 Windows / POSIX 差异。用于 managed output 目录、AGENTS.md 发现、配置加载等。

## 统一接口 `IFileSystem`
- `Read(path) -> Result<std::string, FsError>`：文本读取，UTF-8。
- `Write(path, content) -> Result<void, FsError>`：文本写入（平台相关：Windows 归一化为 CRLF，POSIX 保持 LF）。
- `ReadBinary(path) -> Result<std::vector<std::byte>, FsError>`：二进制读取。
- `WriteBinary(path, data) -> Result<void, FsError>`：二进制写入。
- `Exists(path) -> bool`
- `ListDir(path) -> Result<std::vector<std::string>, FsError>`
- `CreateDirectories(path) -> bool`

## 分平台实现（编译期选用）
模块接口 `fs.cppm` 仅声明 `IFileSystem` / `Result` / `CreateFileSystem()` 工厂；平台实现已并入模块实现单元 `fs/src/fs.cpp`，按 `SILICON_PLATFORM_*` 宏（`顶层 xmake.lua` 定义）在编译期选用对应平台分支：
- `Win32FileSystem`（`fs/src/fs.cpp`，`SILICON_PLATFORM_WINDOWS` 分支）：文本写入归一化为 CRLF；目录创建使用系统默认权限。
- `PosixFileSystem`（`fs/src/fs.cpp`，`SILICON_PLATFORM_UNIX` 分支）：文本保持 LF；目录创建后显式设置 0755 权限。
- 二者均继承共享基类 `FileSystemBase`（同文件，基于 `std::filesystem`），平台差异通过 `NormalizeText()` 钩子与 `CreateDirectories()` 重写下放。

## 对外入口
- `CreateFileSystem() -> std::unique_ptr<IFileSystem>`：工厂，返回当前平台的实现，供调用方以接口形式使用，无需感知具体平台类型。
- 错误以 `Result<T, FsError>` 表达（fs 模块自有 bool 风格 `Result`，非异常）。

## 平台
平台相关实现（Win32 / POSIX）与共享基类 `FileSystemBase` 均位于模块实现单元 `fs/src/fs.cpp`，按宏在编译期选择分支；`CreateFileSystem()` 定义亦在该单元补齐。仅编译期按平台选择，无运行时分派、无独立平台翻译单元。
