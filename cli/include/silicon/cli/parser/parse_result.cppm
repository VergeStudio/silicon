module;
#include <memory>

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

  public:
    ParseResult();
    ~ParseResult();

    ParseResult(const ParseResult &o);
    ParseResult &operator=(const ParseResult &o);
    ParseResult(ParseResult &&) noexcept;
    ParseResult &operator=(ParseResult &&) noexcept;

  public:
    std::string &Command();
    const std::string &Command() const;
    std::map<std::string, std::string> &Flags();
    const std::map<std::string, std::string> &Flags() const;
    std::vector<std::string> &Positional();
    const std::vector<std::string> &Positional() const;

  private:
    /// Implementation state, fully hidden in the implementation unit (src/parser/parse_result.cpp).
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

} // namespace silicon::cli
