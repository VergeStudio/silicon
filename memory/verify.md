# 3. 验证闭环（scripts/verify.sh）

- 用法：`verify.sh`（全量 clean release）/ `--inc` / `--no-build` / `--debug`（debug 后自动还原 release）。
- **门槛（退出码 0）**：构建成功 + **0 warning** + **9 组测试全绿** + dylib **无测试符号泄漏**（`test_main`/`doctest`）。
- macOS 须把 brew LLVM `bin` 置于 PATH 最前（Apple clang 无 std modules 会报 `'map' file not found`）；跑测试须 `DYLD_LIBRARY_PATH` 指向 dylib 目录。
- xmake 3.1.0 `clean -a` 后首跑会报一次良性 config 顺序错，**build 需跑两次、以第二次为准**。
- 增量构建会掩盖未重编模块的 warning，默认走 clean 全量。
