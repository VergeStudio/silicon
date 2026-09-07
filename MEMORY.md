# MEMORY — silicon 工程长期记忆

> 本文件沉淀 silicon 工程的**长期稳定知识与约定**，供团队成员跨会话复用，避免重复踩坑。
> 规则：**每轮修改完，若本轮产生新的约定 / 踩坑经验 / 架构要点，请同步沉淀到这里**（推送仓库 + 项目资产）。
> 与《PROGRESS.md》分工：MEMORY = 稳定知识/约定；PROGRESS = 动态进度快照。

- 维护入口：`/workspace/silicon/MEMORY.md`
- 更新约定：见仓库根 `MEMORY.md` 头部 + 项目资产《silicon接入说明》"团队协作要求"一节。

---

## 1. 仓库与协作基线

- **仓库**：`git.code.tencent.com/VergeStudio/silicon.git`（工蜂）。
- **唯一开发分支**：`leo/dev`（真工程，约 13.8 万行 C/C++）；`master` 是**空壳**（只有空 README），**永远不要在 master 开发、不要拉 master**。
- **认证**：每位成员用自己的工蜂账号 + 访问令牌（`read_repository` 起；要推送需更高权限）。令牌等同密码，**不得进代码/文档/聊天记录**。
- **克隆**：`git clone --branch leo/dev https://<账号>:<令牌>@git.code.tencent.com/VergeStudio/silicon.git`。
- **每轮提交推送约定**：每轮修改完自动 `add + commit + push origin leo/dev`；语义化提交信息（`feat(scope): ...` / `refactor(scope): ...` / `fix` / `chore` / `style` / `test` 等）；若推送被拒先 `git pull --ff-only` 再推。
- **进度/记忆双同步约定**：见下方第 6 节。

## 2. 技术栈与构建

- **语言**：C17 + C++23 Modules；跨平台（Linux / macOS / Windows），单 DLL 伞宏闸门（`CORE_API`/`CORE_EXPORT`）。
- **构建**：xmake（要求 ≥ 3.0.0）。工具链：非 Windows → `clang`；Windows → `msvc`。
- **模块划分**：`core`（主运行时，接口 `core/include/silicon/<mod>/`、实现 `core/src/<mod>/`、测试 `core/test/<mod>/`），`ai` 独立（moduleonly + static，见下）。
- **core 内含模块**（18 个实现目录）：cli、config、coroutine、event、ffi、fs、http、json、logger、network、platform、plugin、scheduler、shared_library、time、tui、util、xdg。
- **AI 模块**：`silicon::ai`（moduleonly 接口）+ `silicon::ai.impl`（static 实现）+ `silicon::ai.test`；LLM 相关在 `include/silicon/ai/llm/*`、`src/ai/llm/*`、`specs/llm.md`。
- **产物布局**：`build/<plat>/<arch>/<mode>/silicon/*.test`（depth 3）；core 出 `libsilicon_core*.dylib`。

## 3. 验证闭环（scripts/verify.sh）

- 用法：`verify.sh`（全量 clean release）/ `--inc` / `--no-build` / `--debug`（debug 后自动还原 release）。
- **门槛（退出码 0）**：构建成功 + **0 warning** + **9 组测试全绿** + dylib **无测试符号泄漏**（`test_main`/`doctest`）。
- macOS 须把 brew LLVM `bin` 置于 PATH 最前（Apple clang 无 std modules 会报 `'map' file not found`）；跑测试须 `DYLD_LIBRARY_PATH` 指向 dylib 目录。
- xmake 3.1.0 `clean -a` 后首跑会报一次良性 config 顺序错，**build 需跑两次、以第二次为准**。
- 增量构建会掩盖未重编模块的 warning，默认走 clean 全量。

## 4. 关键架构决策（详见 docs/ADR-0001~0003，均已采纳落地）

- **proxy v4 类型擦除**（ADR-0001）：vendor 微软 proxy v4（`proxy_macros.h`）；plugin/scheduler/fs/di/http/logger/network/ai.llm/cli 等 facade 均以 `proxy<Facade>` 暴露非侵入式接口契约，规避跨 DLL/ABI 边界的虚表布局耦合。
- **scheduler/task 自研**（ADR-0002）：不走 stdexec P2300；自有 `scheduler::task<T>`（libcoro 风格协程任务，与 io_scheduler 异步栈域耦合，`when_any` 超时/竞速取消）+ `expected<T,E>`（`std::expected<T,std::error_code>` 别名，错误以值传递不抛异常）+ `scheduler_facade`（proxy 类型擦除调度契约：spawn_detached/spawn_joinable/resume/shutdown/size/empty；thread_pool/io_scheduler/inline_scheduler/parallel_scheduler/run_loop 鸭子类型满足）。
- **completion I/O 分层**（ADR-0003）：readiness 层 = `io_notifier`（epoll/kqueue/IOCP 就绪事件 + `poll_info` 状态机），只能服务 socket/pipe/timer；**常规文件不可 readiness 轮询**（epoll `EPERM` / WSAPoll `WSAENOTSOCK`），只能走 completion 引擎或线程池。第二层 = `io_ring`（Linux io_uring / Windows I/O Ring，支持带偏移 `read_at`/`write_at` + CQE 收割），已接入 io_scheduler。

## 5. 踩坑与约定（重要）

- **仓库代码已整体去除注释**（2026-09-04 起，commit `5fd2bf7`/`7d883a8`）：改动时不要依赖旧注释；新增代码是否需要/允许注释请遵循当前代码风格（默认从简，关键处可用自注释命名 + 少量注释）。
- 常规文件 async I/O 不可回退 readiness 模型（硬约束），涉及文件读写一律走 completion 路径或线程池。
- proxy v4 / 类型擦除是跨模块统一接口范式，新增模块门面优先考虑 `proxy<Facade>`，而非虚基类接口。
- 提交信息走仓库既有语义化风格（见上方 git log）。

## 6. 团队协作要求（写入《silicon接入说明》）

**每位成员在每轮修改完成后，必须同步两份工作文档：**
1. **进度 PROGRESS.md**（仓库根 + 项目资产）——把本轮进展、提交 hash、待办登记进"变更日志"，让下一位成员可续接。
2. **记忆 MEMORY.md**（仓库根 + 项目资产）——把本轮产生的稳定知识（新约定/新架构要点/新踩坑）沉淀进对应章节。

> 具体执行路径与"同步到项目资产"的操作方式，以项目《silicon接入说明》"团队协作要求"一节为准。
