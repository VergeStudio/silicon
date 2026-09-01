// parser 已折为 header-only（见 core/include/silicon/cli/parser/parser_types.h）：
// 全部方法体（含 parse）现为头内 inline 全局实体，每个消费 TU 本地发射 weak 符号，
// 规避 MSVC 模块标签 mangling 与 clang 消费方的跨工具链链接未定义。
// 本实现单元原为 out-of-line 方法定义，现已无定义可承载，仅保留模块实现单元壳。
module silicon.cli.parser;
