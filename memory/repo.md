# 1. 仓库与协作基线

> 本文件是**代码仓库接入细节的唯一权威来源**：项目根 `README.md` 只留一句"代码从仓库 `leo/dev` 拉取、细节见本文件",不再重复下列地址/分支/认证/克隆命令。改动仓库接入方式时只改这里。

- **仓库**：`git.code.tencent.com/VergeStudio/silicon.git`（工蜂）。
- **唯一开发分支**：`leo/dev`（真工程，约 13.8 万行 C/C++）；`master` 是**空壳**（只有空 README），**永远不要在 master 开发、不要拉 master**。
- **认证**：每位成员用自己的工蜂账号 + 访问令牌（`read_repository` 起；要推送需更高权限）。令牌等同密码，**不得进代码/文档/聊天记录**。
- **克隆**：`git clone --branch leo/dev https://<账号>:<令牌>@git.code.tencent.com/VergeStudio/silicon.git`。
- **每轮提交推送约定**：每轮修改完自动 `add + commit + push origin leo/dev`；语义化提交信息（`feat(scope): ...` / `refactor(scope): ...` / `fix` / `chore` / `style` / `test` 等）；若推送被拒先 `git pull --ff-only` 再推。
- **进度/记忆双同步约定**：见 [collab.md](./collab.md) 第 6 节。
