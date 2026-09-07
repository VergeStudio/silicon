# 5. 踩坑与约定（重要）

- **仓库代码已整体去除注释**（2026-09-04 起，commit `5fd2bf7`/`7d883a8`）：改动时不要依赖旧注释；新增代码是否需要/允许注释请遵循当前代码风格（默认从简，关键处可用自注释命名 + 少量注释）。
- 常规文件 async I/O 不可回退 readiness 模型（硬约束），涉及文件读写一律走 completion 路径或线程池。
- proxy v4 / 类型擦除是跨模块统一接口范式，新增模块门面优先考虑 `proxy<Facade>`，而非虚基类接口。
- 提交信息走仓库既有语义化风格（见上方 git log）。

**项目网盘(资产)同步踩坑**：
- 网盘对**显示名以 `.md` 结尾**的文件，同名覆盖(overwrite)上传会失效：平台会把 `file_name="X.md"` 拆成 ext=`md`、显示名=`X`，匹配不上既有条目(其显示名为字面 `X.md`)，于是**新增**一条而非覆盖。因此每次覆盖 `PROGRESS.md`/`MEMORY.md` 这类文件都会制造重复条目，违反"默认不新建"。
- 规避：asset 侧这些入口/索引文件的**显示名不带 `.md`**（如 `repo`/`collab`/`conventions`，ext 单独为 md），对它们上传 `file_name="X.md"` 可正常覆盖，不会新增。
- 对**显示名确实含 `.md`** 的文件（如 `progress/PROGRESS.md`、`memory/MEMORY.md`），更新资产时应**优先改仓库并 push**，不要用 overwrite 直传同名去"覆盖"——若要直传，先用 `rename` 把同名旧条目改名腾出规范名，再上传新内容并改名回 `PROGRESS.md`/`MEMORY.md`，残留旧备份只能由有删除权限的成员清理（普通成员 roleID 无 `can_delete`）。
