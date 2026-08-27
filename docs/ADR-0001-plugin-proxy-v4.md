# ADR-0001：plugin 模块采用 Microsoft proxy v4 类型擦除

- 状态：**已采纳（Accepted），代码已落地**
- 日期：2026-08-27
- 范围：plugin 模块接口范式（跨 DLL/ABI 边界的插件视图）
- 关联：`plugin/include/silicon/plugin/facade.cppm`、`core/include/silicon/core/proxy/proxy_macros.h`

## 背景（Context）

插件天然跨越 **DLL / ABI 边界**：宿主与插件常由不同编译单元、甚至不同编译器版本产出。
传统 `IPlugin` 虚基类方案依赖**共享虚表布局**——一旦编译器/RTTI/虚表约定变化即不兼容，且要求
目标类型继承固定基类（侵入式）。

silicon 已在 `core/proxy/proxy_macros.h` vendor **Microsoft proxy v4**（`__msft_lib_proxy4 202603L`，
版权 Microsoft + Next Gen C++ Foundation），并被 scheduler/fs/di/http/logger/network/ai.llm/cli 等多个
facade 采用。问题：plugin 模块应以何种接口范式定义插件契约？

## 决策（Decision）

**plugin 模块采用 proxy v4 作为类型擦除接口范式**，以 `proxy<PluginFacade>` 暴露非侵入式插件视图，
与既有注册表并存（双轨）。已全量落地，`plugin.test` 构建通过。

落地形态（`plugin/include/silicon/plugin/facade.cppm`）：

- **门面**：`plugin_facade : silicon::proxy::facade_builder
  ::add_convention<MemPluginName, std::string_view() const>
  ::add_convention<MemPluginOnLoad, bool()>
  ::add_convention<MemPluginOnUnload, bool()>
  ::add_convention<MemPluginOnReload, bool()>::build{}`
  （`PRO_DEF_MEM_DISPATCH` 定义各成员派发器）。
- **拥有所有权句柄**：`plugin_proxy = silicon::proxy::proxy<plugin_facade>`（32B 胖指针 + vtable 值）。
- **非拥有视图**：`plugin_view = silicon::proxy::proxy_view<plugin_facade>`（24B）。
- **工厂**：`make_plugin<T>(args...)` 就地构造并擦除；`make_plugin_view<T>(target)` 为既有对象建视图。
- **双轨注册表**：
  - `plugin_registry`：`std::map<std::string, plugin_proxy>` 持有，保留 `register_plugin`/`emplace`/
    `get_plugin`/`remove_plugin`/`list_plugins` 的 shared_ptr 风格查询 API。
  - `proxy_plugin_registry`：同样按值持有 `plugin_proxy`，提供 `register`/`emplace`/`get`/`remove`/`list`。
  - 两者底层均以 `plugin_proxy` 值持有，跨 DLL/ABI 边界安全。
- **非侵入桥接**：既有的"具约定成员的类型"（含 `std::shared_ptr<X>`，只要 X 有 `name()/on_load()/...`）
  天然满足 `plugin_facade`，无需改造即可 `make_plugin<X>` 或 `make_plugin_view`。

> 注：全仓 grep 已确认**无遗留旧 `IPlugin` 基类**——proxy 门面即当前唯一接口，"双轨"体现于上述两套
> 注册表（shared_ptr 风格查询 API vs proxy 值持有），而非旧虚基类并存。

## 权衡（Trade-offs）

### 优势（Pros）
- **非侵入**：目标类型无需继承任何基类，鸭子类型满足约定即可擦除（含 `shared_ptr<X>` 桥接）。
- **ABI 稳定**：proxy 用"胖指针 + vtable 值"替代共享虚表，不依赖跨边界虚表/RTTI 布局，跨 DLL 安全。
- **值语义**：小对象内联、无堆分配；`plugin_proxy` 用法与指针一致（`p->name()`、`if (p)`）。
- **可组合**：约定（convention）可自由增删，门面集中描述接口，消费方零耦合。
- **与 silicon 一致**：复用已 vendor 的 proxy v4 与 `scheduler_facade` 同范式，降低认知负担。

### 代价（Cons）
- **宏不随模块导出**：`PRO_DEF_MEM_DISPATCH` 等宏须在消费方全局模块片段**文本包含** `proxy_macros.h`
  再 `import silicon.proxy`（已在 facade.cppm 头部处理）。
- **体积略大**：`plugin_proxy` 32B / `plugin_view` 24B，大于裸 8B 指针；热路径优先用 `plugin_view`。
- **间接调用开销**：派发为一次间接调用，与虚函数量级相当，但避免共享虚表；对极端热路径需评估。
- **调试可见性**：类型擦除调用栈不如具体类型直观；需配合 `PRO4D_DEBUG`/命名约定定位。
- **库依赖**：proxy v4 以 `proxy_macros.h` 文本形式 vendor，需随 silicon 维护版本（当前 202603L）。

### 对比（vs virtual / template）
| 方案 | 侵入性 | 跨 DLL/ABI | 运行时成本 | 适用边界 |
|------|--------|-----------|-----------|---------|
| virtual `IPlugin` | 侵入（须继承） | 脆弱（共享虚表） | 间接调用 | 同编译器同模块 |
| template 约束 | 非侵入 | 不可（编译期耦合） | 零成本 | 同编译单元 |
| **proxy v4** | **非侵入** | **稳定（胖指针）** | **间接调用+小对象内联** | **跨 DLL/ABI 边界** |

## 后果（Consequences）

- plugin 模块接口范式锁定为 proxy v4；新增插件只需满足 `plugin_facade` 成员约定，无需改接口。
- 跨 DLL/ABI 插件加载安全，无需维护虚基类兼容性。
- 后续若需暴露更多插件能力（如 `on_tick`/`config`），在 `plugin_facade` 增 convention 即可。

## 参考

- `plugin/include/silicon/plugin/facade.cppm`（完整门面 + 双轨注册表）
- `plugin/include/silicon/plugin/error.cppm`（`silicon.plugin.error` 独立错误域）
- `core/include/silicon/core/proxy/proxy_macros.h`（vendor 的 Microsoft proxy v4）
- 同范式参考：`scheduler/include/silicon/scheduler/facade.cppm`（`scheduler_facade`）
