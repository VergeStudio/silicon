module;

#include <expected>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

export module silicon.cli.parser;

#include "silicon/cli/common.h"
export import silicon.cli.parser.interface;
export import silicon.cli.parser.parse_result;

export namespace silicon::cli {
/// 默认 CLI 解析器
CLI_API class Parser: public IParser {
  public:
    /// 声明合法子命令。Parse 会校验首个位置参数是否落在已知子命令集合内，
    /// 未登记任何子命令时该维度不做校验（宽松通过）。
    void AddSubcommand(std::string name);
    /// 声明合法命名标志。requires_value=true 时该 flag 必须携带值，否则返回 kMissingArgument。
    /// 未登记任何 flag 时该维度不做校验（宽松通过）。
    void AddFlag(std::string name, bool requires_value = false);

    /// 解析 argv；成功返回 parse_result，失败返回 cli_error 对应的 error_code。
    std::expected<parse_result, std::error_code> Parse(int argc, const char *const *argv) const override;

  private:
    std::set<std::string> subcommands_;
    std::map<std::string, bool> flags_; // name -> requires_value
};

} // namespace silicon::cli
