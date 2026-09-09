# Silicon 工具链：codebase-memory-mcp + agent-lsp

本文件记录 silicon 项目接入的两个 MCP 工具（来自资料库 `vergestudio/tool`）。
两者均为 MCP server，供支持 MCP 的 AI 客户端（Claude Code / CodeBuddy Code CLI / Cursor 等）在
`/workspace/silicon` 内加载使用。

## 二进制位置（已在 PATH 上）

| 工具 | 版本 | 路径 |
|---|---|---|
| `agent-lsp` | 0.19.2 | `/usr/local/bin/agent-lsp`（源自 `vergestudio/tool/agent-lsp_linux_amd64.tar.gz`） |
| `codebase-memory-mcp` | 0.10.8 | `/usr/local/bin/codebase-memory-mcp`（源自 `vergestudio/tool/codebase-memory-mcp-linux-amd64.tar.gz`） |

原始压缩包保留在 `/workspace/tools/`，解包目录同上。

## MCP 配置

项目根已写入 `.mcp.json`，启用两个 server：

- `lsp` → `agent-lsp c:clangd cpp:clangd`（C17 / C++23 走 clangd，已确认 `/usr/bin/clangd` 18.1.3 可用）
- `codebase-memory` → `codebase-memory-mcp`（代码库语义记忆）

AI 客户端在 `/workspace/silicon` 打开项目时会自动读取 `.mcp.json` 并拉起这两个 server。

## 手动启动（HTTP+SSE，便于调试 / 远程 agent 接入）

```bash
# LSP 编排层（HTTP 模式，端口 8080）
agent-lsp --http --port 8080 c:clangd cpp:clangd

# 代码库记忆（需先建索引，见下）
codebase-memory-mcp
```

## 代码库记忆索引

```bash
# 构建（moderate：类型感知 + 相似/语义边；首次约数分钟，取决于 clangd 索引速度）
codebase-memory-mcp cli index_repository --repo-path /workspace/silicon --mode moderate --name silicon --persistence true

# 索引产物写回 .codebase-memory/graph.db.zst，可随仓库共享给队友（免重复全量索引）
# 查询示例：
codebase-memory-mcp cli query_graph  '<json>'
codebase-memory-mcp cli search_code  --repo-path /workspace/silicon --query "scheduler io engine"
codebase-memory-mcp cli list_projects
```

> 注意：`graph.db.zst` 是生成物，建议加入 `.gitignore` 或在资料库记忆中说明，勿随 `leo/dev` 提交。

## 验证状态（初始化时）

- `agent-lsp doctor`：clangd 自动识别为 `c`/`cpp` 后端，LSP initialize/shutdown 正常。
- `codebase-memory-mcp --version`：0.10.8，暴露 `index_repository`/`search_graph`/`query_graph` 等 43 个 client 面。
- `clangd` 18.1.3、`clang` 18.1.3、`cmake` 均已在 `/usr/bin`；`xmake` 缺失（硅验证脚本 `scripts/verify.sh` 需要，后续按记忆单独安装）。
