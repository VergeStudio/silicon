# silicon 代码仓库接入说明

> 本说明供本团队所有成员在 WorkBuddy 云端获取 silicon 代码使用。
> 本文件**仅含地址与命令,不含代码副本**。

## 仓库地址
```
https://git.code.tencent.com/VergeStudio/silicon.git
```

## ⚠️ 最重要:必须拉取 `leo/dev` 分支
仓库有两个分支:
- `master`(默认):**空壳**,只有空的 README,无代码
- `leo/dev`:**真正的工程**(约 13.8 万行 C/C++)

直接 clone 而不指定分支会拉到空的 `master`。请务必克隆 `leo/dev`:

```bash
git clone --branch leo/dev \
  https://<你的工蜂账号>:<你的访问令牌>@git.code.tencent.com/VergeStudio/silicon.git
```

若已误拉 `master`,补救:
```bash
git fetch origin leo/dev
git checkout leo/dev
```

## 认证
- 使用**你自己的工蜂账号**及其访问令牌(Private Token)。
- 需确保你自己的工蜂账号对 `VergeStudio/silicon` 仓库有访问权限;若无权限,请联系仓库管理员将你的账号添加为成员(至少 `Reporter`)。
- 令牌获取:工蜂 → 账号 **设置 → 访问令牌**,勾选 `read_repository`。
- 令牌等同于账号密码,**不要提交进代码、不要写进文档或聊天记录**。

## 说明
- 每位成员的 WorkBuddy 云端沙箱相互独立,首次使用需 clone 一次;clone 后代码在本人的工作区中,一般无需重复。
- 仓库概况:silicon 是通用基础库;技术栈 C17 + C++23 Modules + xmake;构建用 `xmake`,本地验证 `scripts/verify.sh`。
- 不要在 `master` 上开发,所有工作基于 `leo/dev`。

## 每轮进行前:先读取并同步「记忆」与「进度」
每位成员在开启一个新的工作轮次**之前**,**必须先读取**「记忆」与「进度」文件,
同步当前进度和记忆后再动手。这些文件在仓库 `leo/dev` 与本项目资产两处各有一份,**内容一致**(见下方目录结构):

- **记忆(`memory/` 目录,与 WorkBuddy 本地记忆同构)**:入口索引为 `memory/MEMORY.md`
  (总览 + 主题索引表),内容按主题拆分为 `repo.md`(仓库与协作基线)、`build.md`(技术栈与构建)、
  `verify.md`(验证闭环)、`adr.md`(关键架构决策)、`conventions.md`(踩坑与约定)、
  `collab.md`(团队协作要求)。先读 `memory/MEMORY.md`,再按本轮任务**按需读取**相关主题文件;
- **进度(`progress/PROGRESS.md`)**:掌握当前进度快照、进行中主线与验证基线,保证上下文续接;
- 在动手修改前,先将本轮的目标与进展预期**同步写入**上述文件(进度置顶追加本轮计划、
  记忆补充本轮涉及的新约定),轮后**自动上传**回资产对应位置、并随代码 commit+push 到仓库,
  使团队其他成员的沙箱在后续轮次也能读到。

> 配合下方「每轮修改后」的同步要求,形成「**轮前读取 → 轮中更新 → 轮后同步**」的闭环。

## 团队协作要求:每轮修改后同步「进度」与「记忆」
每位成员在每个**工作轮次**内对仓库代码/文档作出修改并提交推送后,**必须同步维护记忆与进度文件**,
以保证团队各成员的 WorkBuddy 云端沙箱(相互独立)能续接上下文、不重复踩坑:

1. **进度**:保存在 `progress/PROGRESS.md`,记录工程当前进度快照、进行中主线、验证基线、
   以及按轮追加的变更日志(每轮提交 hash + 概述,最新在顶部)。
2. **记忆**:保存在 `memory/` 目录,按主题拆分沉淀长期稳定的工程知识/约定/踩坑经验——
   入口索引 `memory/MEMORY.md` + 主题文件
   `repo.md` / `build.md` / `verify.md` / `adr.md` / `conventions.md` / `collab.md`。

**目录结构(仓库 `leo/dev` 与项目资产完全一致,互为镜像、无单文件权威副本)**:
```
leo/dev(仓库根)  ⇄  项目资产(网盘根)/
├── README.md            ← 本说明
├── memory/              ← 记忆(与 WorkBuddy 本地记忆同构,按主题拆分)
│   ├── MEMORY.md        ← 入口索引:总览 + 主题索引表
│   ├── repo.md          ← 1. 仓库与协作基线
│   ├── build.md         ← 2. 技术栈与构建
│   ├── verify.md        ← 3. 验证闭环(scripts/verify.sh)
│   ├── adr.md           ← 4. 关键架构决策(ADR-0001~0003)
│   ├── conventions.md   ← 5. 踩坑与约定
│   └── collab.md        ← 6. 团队协作要求
└── progress/
    └── PROGRESS.md      ← 进度快照 + 变更日志
```

**同步要求**:
- **提交顺序:每轮结束提交进度/记忆时,先「更新」既有、确有必要才「追加」新增**——同一主题/同一天/同一轮的记录若已存在,应**先就地更新**原条目(补充、修订其内容),避免在文件里反复另起新条目造成重复堆积;确属新的主题/新的进展阶段时才**追加**新条目。与「默认不新建」原则(memory/collab.md)呼应:整体上先改既有文件,条目层上先改既有行。
- 仓库侧 `memory/` 与 `progress/PROGRESS.md` 已纳入 `leo/dev` 仓库(修改后随代码一起 commit + push 到 `leo/dev`)。
- **每轮结束后自动上传/覆盖到本项目网盘资产**:进度覆盖 `progress/PROGRESS.md`;
  记忆更新 `memory/` 下对应主题文件,并在主题增减时同步维护 `memory/MEMORY.md` 索引,
  保证资产侧与仓库侧(仓库根 `memory/`、`progress/`)内容一致。
- 资产侧便于其他成员/会话在不 clone 仓库时也能按主题读到记忆与最新进度;两侧均为同一套拆分结构,
  **无"仓库根单文件 MEMORY.md 权威副本"这一中间层**。
- 若本轮没有产生新的进度或稳定知识,可在相应文件注明"本轮无变更",以保持时间线可追溯。
