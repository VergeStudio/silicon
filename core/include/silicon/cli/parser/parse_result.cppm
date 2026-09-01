module;

#include <memory>
#include <map>
#include <string>
#include <vector>

// parse_result 为 header-only（global module 实体），见 parser_types.h 内 mangling 说明；
// 其内部已包含 cli_error_defs.h。本模块仅保留导出壳，供 legacy
// `import silicon.cli.parser.parse_result` 引用。
#include <silicon/cli/parser/parser_types.h>

export module silicon.cli.parser.parse_result;
