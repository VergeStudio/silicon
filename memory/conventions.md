# 5. 踩坑与约定（重要）

- **仓库代码已整体去除注释**（2026-09-04 起，commit `5fd2bf7`/`7d883a8`）：改动时不要依赖旧注释；新增代码是否需要/允许注释请遵循当前代码风格（默认从简，关键处可用自注释命名 + 少量注释）。
- 常规文件 async I/O 不可回退 readiness 模型（硬约束），涉及文件读写一律走 completion 路径或线程池。
- proxy v4 / 类型擦除是跨模块统一接口范式，新增模块门面优先考虑 `proxy<Facade>`，而非虚基类接口。
- 提交信息走仓库既有语义化风格（见上方 git log）。

**项目资产镜像已切换承载：网盘 → 乐享团队知识库（2026-09-07 起）**：
- **协作镜像只保留两处：仓库 `leo/dev`（权威源）⇄ 乐享团队知识库「silicon」（项目资产镜像）**；原项目网盘(Drive)上的 memory/progress 镜像已**退役**，不再作为同步目标。
- 乐享承载为**页级在线文档**：`memory/`（MEMORY.md 索引页 + adr/build/collab/conventions/repo/verify 主题页）、`progress/PROGRESS.md` 页，结构对齐仓库对应目录。
- 镜像/更新乐享页面用乐享 MCP：建条目 `entry_create_entry`、读页 `block_fetch_page`、写页 `block_update_page`、md 导入建页 `entry_import_content`（无长度限制、勿擅自拆分长文）。
- 乐享**页级在线读改写**规避了下方网盘全部权限坑（无 `can_delete` 约束、无同名 `.md` 覆盖不命中、支持版本化草稿 `draft_save/publish` 与文件历史 `file_list_revisions`/`file_revert_file`）。
- 乐享 MCP 无"创建团队 Space"接口（`knowledge.space` 仅只读）；团队知识库须在乐享前端 `VergeStudio` 团队下新建。**silicon 团队 Space 已于 2026-09-07 建成**（见下方「乐享 silicon 镜像 · 结构与条目 id」索引），此后再建镜像直接按 id 操作即可，无需在前端重复创建。
- 页面状态 `status`：导入新页后可能出现 `processing`（后台索引解析中）；**内容实际已可读、可写回**，不影响 `block_fetch_page`/`block_update_page`，索引延迟不必等待（build.md 2026-09-07 即处此状态仍可完整读回）。

**乐享 silicon 镜像 · 结构与条目 id（2026-09-07 建成，镜像更新直接用下列 entry_id 操作）**：
- Space：`silicon`（team `VergeStudio`；space_id `54991653567a4032af214d24ce7972bf`，root_entry_id `2a264c75f3394caea5f79616610a78ae`；前端入口 `https://mcp.lexiang-app.com/spaces/54991653567a4032af214d24ce7972bf`）。访问/读改页统一用下方各 **entry_id**（`block_fetch_page`/`block_update_page`/`entry_describe_entry`）。
- `memory/`（folder entry `f7d01d7fb3414c30baf449405b99e485`）下 7 页：
  - repo.md → `3df951f12e6f4d66990419ab466d94eb`
  - build.md → `7170db40dd094a47b20de4979a952620`
  - verify.md → `5bb3f9dcda9a431fb923a4c9bc979aa7`
  - adr.md → `d2c7284f7da740bca19bc6e8ae8f4b03`
  - conventions.md → `efbb2f1f6afb410ca0bbd523a5b40f83`
  - collab.md → `0f64474307194d8eaf2bcff401444eaa`
  - MEMORY.md → `7ab222c3bf1a449298ee40fbaf5d8b3c`
- `progress/`（folder entry `e9f94e2f69ea430b9afbb2deae2cecb9`）下 1 页：PROGRESS.md → `26125ea6f96e4e0aa963cdc0caaef482`

> 以下为**历史**网盘踩坑（已退役承载，保留备查/若曾需操作旧网盘条目仍适用）：

**项目网盘(资产)同步踩坑（历史）**：
- 核心规律:**该资产条目能否被 API 同名覆盖(overwrite),取决于它的「显示名」是否字面以 `.md` 结尾**,而显示名形态由**创建途径**决定:
  - **API 上传创建**的文件,显示名**不带 `.md`**(ext 单独存 md,如 `collab`/`conventions`/`repo`;亦见本次 `progress/PROGRESS`=file `DCaCZTIbfJMK`),对它们 upload `file_name="X.md"` 能**正常覆盖、不新增重复**。
  - **网页端/其他途径创建**、显示名字面为 `PROGRESS.md`/`MEMORY.md` 的文件,upload `file_name="X.md"` 会被平台拆成 ext=`md`+名=`X`,匹配不上字面 `X.md`,于是**新增**一条而非覆盖 → 制造重复,违反"默认不新建"。
- 判定技巧:用 API `file_upload` 后看 upload_complete 返回的 file_id 是否等于**既有条目 id**——相等=覆盖成功;不等=新增了重复。
- **恢复/更新资产 `progress/PROGRESS.md`、`memory/MEMORY.md` 这类入口文件的最可靠路径**:请有删除权限成员**先删除**该字面 `.md` 条目,再由任一成员用 API 上传到(空)目录 → 得到的文件显示名即为不带 `.md` 的规范形态,此后即可用 overwrite 正常覆盖跟进更新,不再卡壳。
- 在资产侧有删除权限成员介入前,更新这类文件应**以改仓库并 push 为准**,勿反复用 overwrite 直传,以免堆积无法自行删除的重复条目(普通成员 roleID 无 `can_delete`)。
