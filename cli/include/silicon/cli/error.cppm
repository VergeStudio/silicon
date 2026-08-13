module;

#include <string>
#include <system_error>

export module silicon.cli.error;

export namespace silicon::cli {

/// cli 模块专属错误码枚举（命令行解析语义错误）。
enum class cli_error {
    kParseFailed = 1,
    kUnknownOption,
    kMissingArgument,
    kInvalidValue,
    kUnknownSubcommand,
    kUnknown,
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

} // namespace silicon::cli
