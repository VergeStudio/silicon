module;

#include <expected>
#include <system_error>

export module silicon.cli.parser.interface;

#include "silicon/cli/common.h"
export import silicon.cli.parser.parse_result;

export namespace silicon::cli {

/// cli 模块专属错误码枚举（命令行解析语义错误）。
enum class cli_error {
    kParseFailed = 1,
    kUnknownOption,
    kMissingArgument,
    kInvalidValue,
    kUnknownSubcommand,
};

/// 返回 cli_error 专属 error_category（name() = "silicon.cli"）。
[[nodiscard]] inline const std::error_category &cli_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.cli"; }
        std::string message(int ev) const override {
            switch(static_cast<cli_error>(ev)) {
                case cli_error::kParseFailed: return "cli parse failed";
                case cli_error::kUnknownOption: return "unknown option";
                case cli_error::kMissingArgument: return "option requires an argument";
                case cli_error::kInvalidValue: return "invalid argument value";
                case cli_error::kUnknownSubcommand: return "unknown subcommand";
                case cli_error::kUnknown: return "unknown cli error";
            }
            return "unknown cli error";
        }
    } cat;
    return cat;
}

/// 将 cli_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(cli_error e) noexcept {
    return {static_cast<int>(e), cli_category()};
}

/// 命令行参数解析器抽象
CLI_API class IParser {
  public:
    virtual ~IParser() = default;
    /// 解析 argv；成功返回解析结果，失败（未知子命令 / 非法 flag 等）返回 error_code。
    virtual std::expected<parse_result, std::error_code> Parse(int argc, const char *const *argv) const = 0;
};

} // namespace silicon::cli
