module;

export module silicon.core;

// silicon.core 伞模块：export import core 内全部模块接口，消费方
// `import silicon.core;` 即可访问 core.dll 内所有实体，无需逐个 import。
// 各模块自身的 *.error / 子模块接口一并导出（silicon.cli:config 为
// silicon.cli 的分区，无法跨模块 re-export）。

// :config 分区（silicon.core 版本信息，由 core.config.cppm.in 生成）。
export import :config;

// ── 基础层 ──────────────────────────────────────────────────────────────
export import silicon.platform;
export import silicon.util;
export import silicon.error;
export import silicon.library;          // shared_library
export import silicon.proxy;

// ── 基础设施 ────────────────────────────────────────────────────────────
export import silicon.config;
export import silicon.config.config_value;
export import silicon.config.error;
export import silicon.config.json;
export import silicon.di;
export import silicon.di.error;
export import silicon.event;
export import silicon.event.error;
export import silicon.logger;
export import silicon.logger.error;

// ── 系统与数据 ──────────────────────────────────────────────────────────
export import silicon.fs;
export import silicon.fs.error;
export import silicon.time;
export import silicon.xdg;
export import silicon.json;
export import silicon.json.impl;        // vendored nlohmann/json

// ── 并发与调度 ──────────────────────────────────────────────────────────
export import silicon.coroutine;
export import silicon.coroutine.error;
export import silicon.scheduler;
export import silicon.scheduler.error;
export import silicon.scheduler.task;

// ── 网络 ────────────────────────────────────────────────────────────────
export import silicon.network;
export import silicon.network.error;
export import silicon.http;
export import silicon.http.error;

// ── 扩展与交互层 ────────────────────────────────────────────────────────
export import silicon.plugin;
export import silicon.plugin.error;
export import silicon.cli;
export import silicon.cli.error;
export import silicon.cli.parser;
export import silicon.cli.parser.parse_result;
export import silicon.tui;
export import silicon.tui.error;
