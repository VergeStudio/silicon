module;

#include <expected>
#include <system_error>

export module silicon.cli.parser.interface;
import silicon.cli.error;

#include "silicon/cli/common.h"
export import silicon.cli.parser.parse_result;

export namespace silicon::cli {


/// 命令行参数解析器抽象
CLI_API class IParser {
  public:
    virtual ~IParser() = default;
    /// 解析 argv；成功返回解析结果，失败（未知子命令 / 非法 flag 等）返回 error_code。
    virtual std::expected<parse_result, std::error_code> Parse(int argc, const char *const *argv) const = 0;
};

} // namespace silicon::cli
