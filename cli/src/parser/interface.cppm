module;

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

export module silicon.cli.parser.interface;

#include "silicon/cli/common.h"
export import silicon.cli.parser.parse_result;

export namespace silicon::cli {
/// 命令行参数解析器抽象
CLI_API class IParser {
  public:
    virtual ~IParser() = default;
    virtual ParseResult Parse(int argc, const char *const *argv) const = 0;
};

} // namespace silicon::cli
