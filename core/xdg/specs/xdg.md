# silicon.xdg — XDG Base Directory 规范（跨平台）

`silicon::xdg` 提供跨平台的 XDG 基目录解析，语义遵循
[freedesktop XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html)。

## 用法

```cpp
import silicon.xdg;

auto home  = silicon::xdg::home_dir();     // 用户主目录
auto cfg   = silicon::xdg::config_home();  // 用户配置目录
auto data  = silicon::xdg::data_home();    // 用户数据目录
auto cache = silicon::xdg::cache_home();   // 用户缓存目录
auto state = silicon::xdg::state_home();   // 用户状态目录
auto rt    = silicon::xdg::runtime_dir();  // 运行时目录（可能为空）
auto cdirs = silicon::xdg::config_dirs();  // 系统级配置目录集合
auto ddirs = silicon::xdg::data_dirs();    // 系统级数据目录集合
```

## 解析规则

| 函数 | 环境变量 | POSIX 默认 | Windows 映射 |
| --- | --- | --- | --- |
| `home_dir` | `HOME` / `USERPROFILE` | `getpwuid` | `%USERPROFILE%` |
| `config_home` | `XDG_CONFIG_HOME` | `$HOME/.config` | `%LOCALAPPDATA%` |
| `data_home` | `XDG_DATA_HOME` | `$HOME/.local/share` | `%LOCALAPPDATA%` |
| `cache_home` | `XDG_CACHE_HOME` | `$HOME/.cache` | `%LOCALAPPDATA%\cache` |
| `state_home` | `XDG_STATE_HOME` | `$HOME/.local/state` | `%LOCALAPPDATA%\state` |
| `runtime_dir` | `XDG_RUNTIME_DIR` | （无默认，返回空串） | （返回空串） |
| `config_dirs` | `XDG_CONFIG_DIRS` | `/etc/xdg` | 空 |
| `data_dirs` | `XDG_DATA_DIRS` | `/usr/local/share:/usr/share` | 空 |

macOS 不原生支持 XDG；若设置了对应 `XDG_*` 环境变量则优先采用，否则回退到
`~/Library/Application Support`、`~/Library/Caches` 等约定路径。

环境变量优先于默认路径，因此用户可通过设置 `XDG_CONFIG_HOME` 等自定义位置。
