module;

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

export module silicon.cli.parser.parse_result;

#include "silicon/cli/common.h"

export namespace silicon::cli {
/// 解析结果
CLI_API struct ParseResult {
    std::string command;                      // 子命令名（若无则为空）
    std::map<std::string, std::string> flags; // --key value 或 --flag → "true"
    std::vector<std::string> positional;      // 位置参数
};

} // namespace silicon::cli
