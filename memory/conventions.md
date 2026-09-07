# 5. 踩坑与约定（重要）

- **仓库代码已整体去除注释**（2026-09-04 起，commit `5fd2bf7`/`7d883a8`）：改动时不要依赖旧注释；新增代码是否需要/允许注释请遵循当前代码风格（默认从简，关键处可用自注释命名 + 少量注释）。
- 常规文件 async I/O 不可回退 readiness 模型（硬约束），涉及文件读写一律走 completion 路径或线程池。
- proxy v4 / 类型擦除是跨模块统一接口范式，新增模块门面优先考虑 `proxy<Facade>`，而非虚基类接口。
- 提交信息走仓库既有语义化风格（见上方 git log）。

**项目网盘(资产)同步踩坑**：
- 核心规律:**该资产条目能否被 API 同名覆盖(overwrite),取决于它的「显示名」是否字面以 `.md` 结尾**,而显示名形态由**创建途径**决定:
  - **API 上传创建**的文件,显示名**不带 `.md`**(ext 单独存 md,如 `collab`/`conventions`/`repo`;亦见本次 `progress/PROGRESS`=file `DCaCZTIbfJMK`),对它们 upload `file_name="X.md"` 能**正常覆盖、不新增重复**。
  - **网页端/其他途径创建**、显示名字面为 `PROGRESS.md`/`MEMORY.md` 的文件,upload `file_name="X.md"` 会被平台拆成 ext=`md`+名=`X`,匹配不上字面 `X.md`,于是**新增**一条而非覆盖 → 制造重复,违反"默认不新建"。
- 判定技巧:用 API `file_upload` 后看 upload_complete 返回的 file_id 是否等于**既有条目 id**——相等=覆盖成功;不等=新增了重复。
- **恢复/更新资产 `progress/PROGRESS.md`、`memory/MEMORY.md` 这类入口文件的最可靠路径**:请有删除权限成员**先删除**该字面 `.md` 条目,再由任一成员用 API 上传到(空)目录 → 得到的文件显示名即为不带 `.md` 的规范形态,此后即可用 overwrite 正常覆盖跟进更新,不再卡壳。
- 在资产侧有删除权限成员介入前,更新这类文件应**以改仓库并 push 为准**,勿反复用 overwrite 直传,以免堆积无法自行删除的重复条目(普通成员 roleID 无 `can_delete`)。
