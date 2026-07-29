# Spec: Platform (silicon.platform)

## 职责
平台抽象层——把 OS 相关差异（OS 名、路径分隔符、换行符等）收敛到统一接口之后。
落实第 9 项约定：平台相关实现文件用 `_win` / `_linux` / `_unix` 后缀，模块内按编译期
`os` 检测（`silicon::platform::os`，见 `os_id`）做实现选用开关。本模块同时提供
编译期平台探测（`os_id` / `arch_id` / 各 `constexpr` 属性）与运行时抽象（`IPlatform`）。

## 核心类型
- `IPlatform`：运行时平台抽象接口（`os_name` / `path_separator` / `line_ending`）
- `LinuxPlatform` / `WindowsPlatform` / `UnixPlatform`：各平台具体实现
- `os_id` / `arch_id`：编译期 OS / 架构枚举；`os` / `arch`：当前平台的 `constexpr` 值
- 编译期属性：`native_newline()`、`path_separator()`、`shared_lib_prefix/suffix()`、
  `executable_suffix()`、`is_unix_family()` 等

## 接口
```cpp
class IPlatform {
  virtual ~IPlatform() = default;
  virtual std::string os_name() const = 0;
  virtual char        path_separator() const = 0;
  virtual std::string line_ending() const = 0;
};
std::unique_ptr<IPlatform> create_platform();
```

## 实现选择（编译期）
`platform.cppm` 在模块 purview 内按编译器预定义宏（`_WIN32` / `__linux__` / `__unix__` /
`__APPLE__` 等）`#include` 对应的 `_win` / `_linux` / `_unix` 实现头文件，避免多模块
BMI 暴露问题；`create_platform()` 用 `if constexpr (os == ...)` 返回所选具体实现。
模块自包含，不依赖消费项目注入的 `SILICON_PLATFORM_*` 宏。

> 注：消费项目（如 siliconbuddy）在 DI 组合根中仍可基于 `SILICON_PLATFORM_*` 宏把
> 具体实现绑定到 `silicon::platform::IPlatform`，宏由顶层 xmake.lua 按平台定义。

## 不变式
1. 仅一个平台实现被编译进模块（宏保证互斥）。
2. `create_platform()` 返回的 `IPlatform` 在 unix 系上 `path_separator()=='/'`、`line_ending()=="\n"`。
3. linux 构建机上 `os_name()` 返回 `"linux"`；windows 上返回 `"windows"`。
