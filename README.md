# silicon 代码仓库接入说明

> 本说明供本团队所有成员在 WorkBuddy 云端获取 silicon 代码使用。
> 本文件**仅含地址与命令，不含代码副本**。
>
> **记忆与进度唯一权威在 WorkBuddy 原生资料库 `vergestudio/silicon`**（详见下文「记忆与进度在资料库」一节），本仓库**不保留**记忆 / 进度副本。

## 仓库地址
```
https://git.code.tencent.com/VergeStudio/silicon.git
```

## ⚠️ 最重要：必须拉取 `leo/dev` 分支
仓库有两个分支：
- `master`（默认）：**空壳**，只有空的 README，无代码
- `leo/dev`：**真正的工程**（约 13.8 万行 C/C++）

直接 clone 而不指定分支会拉到空的 `master`。请务必克隆 `leo/dev`：

```bash
git clone --branch leo/dev \
  https://<你的工蜂账号>:<你的访问令牌>@git.code.tencent.com/VergeStudio/silicon.git
```

若已误拉 `master`，补救：
```bash
git fetch origin leo/dev
git checkout leo/dev
```

## 认证
- 使用**你自己的工蜂账号**及其访问令牌（Private Token）。
- 需确保你自己的工蜂账号对 `VergeStudio/silicon` 仓库有访问权限；若无权限，请联系仓库管理员将你的账号添加为成员（至少 `Reporter`）。
- 令牌获取：工蜂 → 账号 **设置 → 访问令牌**，勾选 `read_repository`。
- 令牌等同于账号密码，**不要提交进代码、不要写进文档或聊天记录**。

## 说明
- 每位成员的 WorkBuddy 云端沙箱相互独立，首次使用需 clone 一次；clone 后代码在本人的工作区中，一般无需重复。
- 仓库概况：silicon 是通用基础库；技术栈 C17 + C++23 Modules + xmake；构建用 `xmake`，本地验证 `scripts/verify.sh`。
- 不要在 `master` 上开发，所有工作基于 `leo/dev`。

## 记忆与进度在资料库（唯一权威）
团队的「记忆」与「进度」工作文档**不存放在本仓库**，其唯一权威承载是 **WorkBuddy 原生资料库**：

> **`vergestudio/silicon`**（WorkBuddy 资料库：`我的文档`/团队空间 → `vergestudio` → `silicon`）

资料库内结构（权威，成员直接在其中读写）：
```
vergestudio/silicon（资料库目录）
├── README            ← 唯一权威说明入口（内容与本文件一致）
├── memory/           ← 记忆（按主题拆分，入口索引 MEMORY）
│   ├── MEMORY        ← 入口索引：总览 + 主题索引表
│   ├── repo          ← 1. 仓库与协作基线
│   ├── build         ← 2. 技术栈与构建
│   ├── verify        ← 3. 验证闭环
│   ├── adr           ← 4. 关键架构决策
│   ├── conventions   ← 5. 踩坑与约定
│   └── collab        ← 6. 团队协作要求
└── progress/
    └── PROGRESS      ← 进度快照 + 变更日志
```

**协作闭环（全部在资料库进行，不涉及本仓库）**：
- 每位成员开启新一轮工作**前**，先在资料库读取 `memory/MEMORY`（入口索引）与 `progress/PROGRESS`，按需读取对应主题，掌握进度与记忆基线；
- 动手修改代码前，把本轮目标与进展预期**同步写入**资料库的进度（`progress/PROGRESS` 置顶追加本轮计划）与记忆（`memory/` 对应主题）文档；
- 修改代码后 commit + push 到本仓库 `leo/dev`（仅代码与随代码的 `docs/ADR-*.md` 决策记录）；同时回资料库更新进度 / 记忆文档。

> 本仓库代码改动的历史仍以 git 提交为准；`docs/ADR-*.md` 是随代码走的架构决策记录，**不**属于上述记忆文档，仍留在本仓库维护。
