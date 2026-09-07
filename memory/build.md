# 2. 技术栈与构建

- **语言**：C17 + C++23 Modules；跨平台（Linux / macOS / Windows），单 DLL 伞宏闸门（`CORE_API`/`CORE_EXPORT`）。
- **构建**：xmake（要求 ≥ 3.0.0）。工具链：非 Windows → `clang`；Windows → `msvc`。
- **模块划分**：`core`（主运行时，接口 `core/include/silicon/<mod>/`、实现 `core/src/<mod>/`、测试 `core/test/<mod>/`），`ai` 独立（moduleonly + static，见下）。
- **core 内含模块**（18 个实现目录）：cli、config、coroutine、event、ffi、fs、http、json、logger、network、platform、plugin、scheduler、shared_library、time、tui、util、xdg。
- **AI 模块**：`silicon::ai`（moduleonly 接口）+ `silicon::ai.impl`（static 实现）+ `silicon::ai.test`；LLM 相关在 `include/silicon/ai/llm/*`、`src/ai/llm/*`、`specs/llm.md`。
- **产物布局**：`build/<plat>/<arch>/<mode>/silicon/*.test`（depth 3）；core 出 `libsilicon_core*.dylib`。
