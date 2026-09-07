# PROGRESS — silicon 开发进度

> 本文件记录 silicon 工程（分支 `leo/dev`）的**开发进度**，供团队各成员/各轮会话延续上下文使用。
> 规则：**每轮修改完，由当轮成员把本轮进展同步到这里**（并在项目根 README.md 约定的协作流下推送到仓库与乐享团队知识库）。
> 与《MEMORY.md》分工：MEMORY = 长期稳定的工程知识 / 约定 / 踩坑经验；本文件 = 正在推进的进展快照。

- 维护入口：仓库 `leo/dev` 下 `progress/PROGRESS.md`（镜像至乐享团队知识库「silicon」`progress/` 页）。
- 更新约定：见仓库根 `README.md`（"每轮进行前"+"团队协作要求"两节，目录结构 `progress/`）。
- 乐享侧视图：与仓库互为镜像（无单文件权威副本），见仓库根 `README.md`。

---

## 当前进度（快照）

> 最近更新：2026-09-07

### 已完成 / 进行中的主线

- **调度器 I/O 引擎分层重构**（近期核心工作线，ADR-0003 已落地）：
  - 引入 **io_ring**（Linux=io_uring / Windows=I/O Ring）平台 completion I/O 封装：`io_ring.cppm` 接口 + `io_ring_uring.cpp` / `io_ring_ioring.cpp` 后端；支持带偏移 `read_at` / `write_at` 与 CQE 收割。默认关闭（`--io_ring=y` 开启），关闭时后端为空 TU、接口单元仍参与编译做跨平台语法校验。
  - **io_ring 已接入 io_scheduler 第二层 completion I/O**（commit `6a319de`），提交带偏移常规文件读写的 completion 路径。
- **scheduler 命名空间归位重构**（commit `a7c3634`）：`scheduler` 去 `silicon::coroutine` 依赖，原语归位 `silicon::scheduler`；非 Windows 文件去掉 MSVC 守卫。
- **代码去注释清理**（commit `5fd2bf7`、`7d883a8`）：全部 C/C++ 源码去除注释（保留字符串/原始字符串字面量，安全解析），并归一多余空行。
- **proxy v4 / scheduler / io_ring 架构决策** 已沉淀为 `docs/ADR-0001 ~ 0003`。

### 验证基线

- 平台目标：macOS / clang 22 为主，兼容 Linux（clang + 可选 liburing）/ Windows（MSVC + I/O Ring）。
- 本地验证：`scripts/verify.sh`（全量 clean 重建 + 9 组测试全绿 + dylib 无测试符号泄漏 + 0 warning）。
- ⚠️ 仓库代码以**去除注释**后的新基线推进，改动时不要依赖旧注释信息。

### 待办 / 下一步（若本轮有新增会在此登记）

- 【本会话·"显式 private"任务】：批1(eef4bd5 核心 pimpl)+批2(eaa6ef4 scheduler/network 补漏)已完成并推送 `leo/dev`。批3/4(vendored) 深入勘察后裁定"无可安全改动对象"→ 保持原样(见变更日志)。剩余:在可访问 github 平台跑 `scripts/verify.sh`(本沙箱无法 xmake 全量;libstdc++13 `<expected>` C++23 缺陷)。
- 【乐享知识库迁移·已完成(2026-09-07)】：协作镜像已定为 仓库 ⇄ 乐享「silicon」两处(网盘退役)。**silicon 团队知识库已在乐享前端 `VergeStudio` 团队下建成**(space_id `54991653567a4032af214d24ce7972bf`),仓库 `memory/`(MEMORY.md+6 主题页)与 `progress/PROGRESS.md` 已全部**页级镜像**进乐享、验证内容可读回。**结构/各页 entry_id 索引见 `memory/conventions.md`「乐享 silicon 镜像 · 结构与条目 id」小节**,后续轮次更新乐享页直接按 id 用 `block_fetch_page`/`block_update_page`/`entry_import_content` 即可,无需前端重复创建(乐享 MCP 无创建团队 Space 接口)。
- 【历史·网盘 progress/ 状态(承载已退役)】：原网盘两个 PROGRESS 条目均已清空、已恢复过镜像;因承载已切乐享,该网盘镜像不再作同步目标(踩坑见 conventions 历史节,仅旧网盘条目操作时适用)。

---

## 变更日志（按轮追加）

> 每轮修改完在此追加一条；格式：`日期 | 提交 | 概述`。**最新在顶部**。

| 日期 | 提交 / 链接 | 概述 |
|------|------------|------|
| 2026-09-07 | `db47e7d` | 乐享 silicon 镜像**落地收尾**:① VergeStudio 团队 silicon 团队 Space 已在乐享前端建成(space_id `54991653567a4032af214d24ce7972bf`,root_entry_id `2a264c75f3394caea5f79616610a78ae`),仓库 `memory/`(MEMORY.md 索引 + adr/build/collab/conventions/repo/verify 6 主题)与 `progress/PROGRESS.md` 共 8 页已用乐享 MCP **页级镜像**进乐享(`entry_import_content` 建页、`block_fetch_page` 读回验证内容保真,PROGRESS 大表格完整;build.md 状态曾 `processing` 实为后台索引延迟,内容可读可写)。② `memory/conventions.md` 措辞更新:L13 由"需在乐享前端新建 silicon 知识库"改为"已建成",新增 status `processing` 说明 + 「乐享 silicon 镜像 · 结构与条目 id」索引小节(登记 space_id/root_entry_id/memory 7 页/progress 1 页全部 entry_id,供后续轮次直接按 id 用 MCP 镜像更新,无需前端重复创建);乐享侧同页已同步。③ 待办节迁移项更新为"已完成"。网盘 memory/progress 镜像此前已退役,仓库⇄乐享两处镜像闭环达成。 |
| 2026-09-07 | `572ee76` | 协作镜像承载切换决策：**只保留 仓库 `leo/dev`(权威源) ⇄ 乐享团队知识库「silicon」(项目资产镜像) 两处**，原项目网盘(Drive)上的 memory/progress 镜像**退役**、不再作同步目标。仓库侧同步更新：根 `README.md`（目录结构/同步要求 + 新增「资产镜像承载:乐享」小节）、`memory/collab.md`（自动覆盖授权→乐享页级镜像授权）、`memory/conventions.md`（登记录用乐享规避网盘权限坑 + 网盘踩坑保留为历史）、`progress/PROGRESS.md`（本记录）。乐享 MCP 已接入(VergeStudio 团队 + 页级 block 在线读写)。**待办**：乐享 MCP 无"创建团队 Space"接口,需在乐享前端 VergeStudio 下建 silicon 团队知识库后,将仓库 memory/ + progress/ 内容页级镜像进乐享(结构对齐目录树)。 |
| 2026-09-07 | `-`(资产镜像恢复记录) | 资产 `progress/` 清理完成：有权限者已删除多余的两个 PROGRESS 条目(旧 `DCaCZTIbfJMK` 与新增的 `DePfxIvuzQAI`),目录清空。本会话据此将仓库最新 PROGRESS.md(含 vendored 终裁 + 本记录)重新上传至 `progress/` 恢复镜像一致,下载 MD5 复核通过。 |
| 2026-09-07 | `-`(无代码改动,结论登记) | "显式 private"任务·vendored(proxy/json)终裁：**无可安全改动对象,保持原样**。深入勘察(证据见下)后与成员确认:① **proxy/impl.cppm**(微软 proxy4)185 个 struct **全部是需 public 供模板推导的元编程 trait**(copyability_traits/reduction_traits/type_identity helper 等),无数据成员、加 private 即破坏 proxy facade 机制;② **json/(nlohmann)** 封装类型数据成员**均已由上游 `private:` / `JSON_PRIVATE_UNLESS_TESTED:` 宏隔离**(basic_json 的 `m_data`/`m_parent` 在宏内;iter_impl/lexer/parser/serializer/各 adapter/json_pointer 等约 26 类均已 private),剩余 public 数据都在 `internal_iterator`/`position_t`/`diyfp` 等**有意设计的公开数据容器/POD/union**(basic_json 内嵌 `data`/`json_value` 已被宏包住),加 private 会破坏库内 friend 互访/序列化宏/算法内聚。两库访问控制均为上游既定设计,非"省略 private",强改零收益且破坏面大。本任务"显式 private"仅落在**本仓库手写代码**(批1/2)。 |
| 2026-09-07 | `-`(资产同步过程记录) | 资产同步遇坑登记：`file_upload` 以 `file_name="PROGRESS"`+`overwrite` 上传时**未命中既有条目 `DCaCZTIbfJMK`(display 名带 `.md` 扩展、由网页端建),反在 progress/ 新建同名无扩展条目 `DePfxIvuzQAI`**,导致目录暂时两个 PROGRESS。新条目内容正确(与仓库 MD5 993b6ee5 一致)。删除旧条目被拒:本账号无 `can_delete` 属性(roleID:22),删除需有权限成员。详见待办节遗留。 |
| 2026-09-07 | `eaa6ef4` | refactor(access): scheduler/network 剩余 pimpl class 补显式 private——补齐 core 遗漏的含 pimpl 实现细节但首段未显式 private 的类型。scheduler: inline_scheduler/io_notifier/io_ring/parallel_scheduler/run_loop/timer_handle(class 默认私有首段,纯显式化)与 poll_info(`struct` 默认 public 泄漏 `m_p`,真修复,已核实所有访问均在成员/嵌套类内);network: hostname/socket_address(class 首段 pimpl,纯显式化;ip_address 本就合规)。逐文件核实其余含 `struct impl;` 前向声明的 core 类型(parser/parse_result/coroutine* 全部/logger/event/event/tcp/udp/pipe/poll/sync_wait/thread_pool/shared_library 等)均已显式位于 private 区段,无需改动。 |
| 2026-09-07 | `eef4bd5` | refactor(access): 全量"显式 private"任务——核心 pimpl struct/class 在实现细节(`struct impl`/`impl_` 指针)前显式加 `private:`(前置空行),构造+访问器留 public。覆盖 core(http/facade、http/types、config/value、plugin/facade、time/facade)+ ai(llm/types 5 个 pimpl struct、llm/facade tool_registry/provider_registry/scripted_provider/http_provider+内嵌 http_result)。其中 ai/types 的 5 struct + http_result 因 **struct 默认 public 致 `struct impl;`+`impl_` 意外泄漏(真缺陷修复)**;core/http/types 的 http_response/http_request、time/facade 的 date_source 同属 struct 泄漏修复。class 各例(class 默认 private)为纯显式化。clang18 最小模块复刻验证:模块实现单元可定义接口单元中 **private 嵌套前向类型** `X::impl`(编译+链接+运行 exit=0),证明该类 pimpl 私有化编译安全。 |
| 2026-09-07 | `600928d` | refactor(logger): 删除未接入的 `global_logger`(`:global_logger` 分区),保留 `default_logger`。global_logger 为孤立死代码:facade.cppm(module `silicon.logger`)只 `export import :ilogger`/`:default_logger`、从不聚合 `:global_logger`;模块级 `init/stop/...` 全局函数在 logger.cpp 委托匿名 default_logger。删 接口 `global_logger.cppm` + 实现 `global_logger.cpp`(共 177 行),全仓无任何引用、内容与 default_logger 重复。其余文件零改动。 |
| 2026-09-07 | `88b687a` | refactor(platform): 实现单元 `core/src/platform/facade.cpp`(module `silicon.platform`,OS/arch 平台类+`create_platform` 工厂)更名 `platform.cpp`,对齐 `src/<mod>/<mod>.cpp` 惯例。仅文件改名,模块名/引用/xmake(glob) 零改动;不改任何 facade.cppm。 |
| 2026-09-07 | `87ba2ed` | refactor(library): `shared_library_impl.h` 迁移——删除单头共享的 `struct shared_library::impl`,将完整 pimpl 定义并入接口单元 `shared_library.cppm`(对齐 http/config_value"接口单元放完整 impl"范式, GMF 补 `<mutex>`);3 个实现单元(shared_library/_unix/_windows.cpp)删除 `#include "shared_library_impl.h"`,impl 改由接口单元提供、同模块实现单元可见。沙箱 clang18 因 libstdc++ `<expected>`(C++23) 头加载缺陷无法对含 expected 文件独立 precompile,已用同构最小模块复刻验证 clang 模块图编译链接运行退出码 0。 |
| 2026-09-07 | `d50b7ec` | refactor(time): `core/include/silicon/time/system_clock.h` 迁移为 C++20 模块——新建接口 `core/include/silicon/time/system_clock.cppm`(`module silicon.time.system_clock`,类标 `SILICON_CORE_API`,`now()`/`now_ms()` 仅声明)+ 实现 `core/src/time/system_clock.cpp` 定义两方法(原 header 内联体搬入);facade.cppm(`module silicon.time`)删除 GMF include 与 `using` re-export、改 `export import silicon.time.system_clock`;core.cppm 伞头补 `export import silicon.time.system_clock;`;原 `.h` 已删,time_test.cpp 去 header include(经 facade re-export 取类型)与 test xmake.lua 注释同步。clang18 独立模块图验证(接口 precompile+实现单元+消费 TU 编译链接运行退出码 0)。注:沙箱无法访问 github 拉 spdlog/c-ares,未跑 xmake 全量构建 |
| 2026-09-07 | `b658143` | 协作规则：登记「资产同步自动覆盖、免逐一确认」授权——每轮修改完成后当轮成员**自动**将改动的记忆/进度覆盖上传到资产对应条目、不再逐次询问；常规同步自动完成并下载+MD5 校验。记入根 `README.md`「同步要求」与 `memory/collab.md`。 |
| 2026-09-07 | `912f3f2` | refactor(core): 单 DLL 伞宏更名——`CORE_API`→`SILICON_CORE_API`、`CORE_EXPORT`→`SILICON_CORE_EXPORT`（统一 SILICON_ 前缀风格）。批量替换约 65 个 core 接口/实现单元 + `common.h` 宏定义 + `core/xmake.lua`/`ai/xmake.lua` add_defines 与注释 + `core.config.cppm.in`；`memory/build.md` 伞宏闸门描述同步。ai 的 `AI_API`/`AI_EXPORT` 独立体系未动。 |
| 2026-09-07 | `9a8e4e3` | refactor(http): `core/include/silicon/http/http_types.h` 迁移为 C++20 模块——新建接口 `core/include/silicon/http/types.cppm`(`module silicon.http.types`) + 实现 `core/src/http/types.cpp`,形态对齐仓库 `config/config_value.cppm`(impl 嵌套 + 拷贝内联、访问器入 .cpp、类标 `CORE_API`);原 `.h` 已删,消费方(facade/http.cpp/http_test/core.cppm)改用 `import`/`export import`;用 clang18 独立模块图验证(接口 precompile+实现单元+消费 TU 编译链接运行,http_response/http_request 行为断言全通过)。注:沙箱无法访问 github 拉 spdlog/c-ares,未跑 xmake 全量构建 |
| 2026-09-07 | `122a980` | 记忆沉淀：conventions 澄清网盘覆盖规律——「能否被 API overwrite 取决于条目显示名是否字面含 `.md`（由创建途径决定：API 建的不含可覆盖；网页端建的字面含不可覆盖）」；记录"有权限者删除→API 重传到空目录→此后可覆盖"的恢复路径 |
| 2026-09-07 | `292a2b9` + `73cdee2` | 协作规则沉淀：约定「每轮只修改既有记忆/进度文件、默认不新建」+「先更新既有、确有必要才追加」，记入 `collab.md`（维护原则）并写入 README「同步要求」，同时沉淀 conventions 网盘 `.md` 同名覆盖失效的踩坑与规避法 |
| ✅ 已解决 | 资产进度同步 | 资产 `progress/PROGRESS.md`（旧 `DqmuDzQsUuOA` 内容落后）已由有权限成员删除，本会话按待办方案用 API 重传最新内容（新 file_id `DCaCZTIbfJMK`，与仓库 `leo/dev` HEAD 内容 MD5 一致，`progress/` 目录唯一无残留）。镜像已恢复一致。 |
| 2026-09-07 | `ccdd4d5` | 核对并确认：仓库 `leo/dev`（HEAD `e29072f`）与项目资产已**完全互为镜像一致**（根 README + memory/ 7 主题 + progress/PROGRESS.md，MD5 逐一匹配）；此前遗留的重复文件（资产 `DKSZEoCbXHVq`/`DLURswbCXMMb`）已由有权限成员清理，无残留。顺带修正 PROGRESS 头部过期的"维护入口"旧路径。 |
| 2026-09-07 | `12f9351` | 对齐 README 更新：资产侧记忆改 memory/ 主题拆分、进度改 progress/；仓库 MEMORY/PROGRESS 更新引用与协作闭环（"轮前读→轮中更新→轮后同步"） |
| 2026-09-07 | `559d59d` | docs: 新增 PROGRESS/MEMORY 工作文档；并同步至项目网盘资产、在接入说明登记"每轮同步进度记忆"协作要求 |
| 2026-09-04 | `7d883a8` | style: 折叠去注释遗留的多余空行（连续空行归一，字面量内空行保留） |
| 2026-09-04 | `5fd2bf7` | chore: 去除全部 C/C++ 源码注释（安全解析，保留字符串/原始字符串字面量） |
| 2026-09-04 | `a7c3634` | refactor(core): scheduler 去 silicon::coroutine（原语归位 silicon::scheduler）+ 非 Windows 文件去 MSVC 守卫 |
| 2026-09-04 | `6a319de` | feat(scheduler): io_ring 接入 io_scheduler 第二层 completion I/O（read_at/write_at） |
| 2026-09-04 | `3ffcb01` | feat(scheduler): 新增 completion I/O 环 io_ring（Linux io_uring / Windows I/O Ring） |
| 2026-09-04 | `04bfc46` | refactor(network): socket 实现按平台分文件（socket_linux / socket_win） |
| （登记处） | — | 请各成员在本轮提交后，把提交 hash + 概述补充到此处顶部 |
