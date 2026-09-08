# Spec: silicon.fs — 文件系统抽象

## 职责
提供跨平台、平台无关的文件系统统一代理接口 `file_system_proxy`（基于 `silicon.proxy` facade），对外隐藏 Windows / POSIX 差异。用于 managed output 目录、AGENTS.md 发现、配置加载等。

## 统一接口（proxy facade）
接口单元 `core/include/silicon/fs/facade.cppm` 定义 `file_system_facade` 约定集：
- `read(path) -> silicon::error::result<std::string>`：文本读取，UTF-8。
- `write(path, content) -> silicon::error::result<void>`：文本写入（平台相关：Windows 归一化为 CRLF，POSIX 保持 LF）。
- `read_binary(path) -> silicon::error::result<std::vector<std::byte>>`：二进制读取。
- `write_binary(path, data) -> silicon::error::result<void>`：二进制写入。
- `exists(path) -> bool`
- `list_dir(path) -> silicon::error::result<std::vector<std::string>>`
- `create_directories(path) -> bool`

## 平台拆分（编译期选用）
按全工程平台拆分规范：平台无关部分位于不带平台后缀的单元，平台实现各自独立成带平台后缀的文件，文件内部以 `SILICON_PLATFORM_*` 宏自守卫（`顶层 xmake.lua` 定义宏）：
- 平台无关共享基类模板 `file_system_base`（`facade.cppm` 非导出区，基于 `std::filesystem`）：read / read_binary / write_binary / write / exists / list_dir / create_directories / normalize_text 钩子。
- `core/src/fs/fs_win.cpp`（`SILICON_PLATFORM_WINDOWS` 守卫）：`win32_file_system`——normalize_text 归一化为 CRLF；工厂 `create_file_system()` Windows 版。
- `core/src/fs/fs_unix.cpp`（`SILICON_PLATFORM_UNIX` 守卫）：`posix_file_system`——文本保持 LF；重写 create_directories 显式设置 0755 权限；工厂 `create_file_system()` POSIX 版。

## 对外入口
- `make_file_system<T>(...)`：模板工厂（proxy），`create_file_system()` 返回当前平台实现。
- 错误以 `silicon::error::result<T>` 表达（统一错误模块，非异常、非 fs 自有别名）。

## 平台
平台实现位于 `fs_win.cpp` / `fs_unix.cpp` 两个独立翻译单元；共享基类位于接口单元非导出区（模块内可见）。仅编译期按平台选择，无运行时分派。
