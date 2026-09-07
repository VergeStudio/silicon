# PROGRESS — silicon 开发进度

> 本文件记录 silicon 工程（分支 `leo/dev`）的**开发进度**，供团队各成员/各轮会话延续上下文使用。
> 规则：**每轮修改完，由当轮成员把本轮进展同步到这里**（并在项目资产根 README.md 约定的协作流下推送到仓库与项目资产）。
> 与《MEMORY.md》分工：MEMORY = 长期稳定的工程知识 / 约定 / 踩坑经验；本文件 = 正在推进的进展快照。

- 维护入口：仓库 `leo/dev` 下 `progress/PROGRESS.md`（项目资产镜像至网盘 `progress/PROGRESS.md`）。
- 更新约定：见仓库根 `README.md`（"每轮进行前"+"团队协作要求"两节，目录结构 `progress/`）。
- 资产侧视图：与仓库互为镜像（无单文件权威副本），见网盘根 `README.md`。

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

- （按需补充：本会话/成员在推进的具体事项、阻塞点、下一步计划）

---

## 变更日志（按轮追加）

> 每轮修改完在此追加一条；格式：`日期 | 提交 | 概述`。**最新在顶部**。

| 日期 | 提交 / 链接 | 概述 |
|------|------------|------|
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
