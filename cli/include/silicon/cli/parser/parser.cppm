module;

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

export module silicon.cli.parser;

#include "silicon/cli/common.h"
export import silicon.cli.parser.interface;
export import silicon.cli.parser.parse_result;

export namespace silicon::cli {
/// 默认 CLI 解析器
CLI_API class Parser: public IParser {
  public:
    ParseResult Parse(int argc, const char *const *argv) const override;
};

} // namespace silicon::cli
