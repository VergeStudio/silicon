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
CLI_API struct parse_result {

  public:
    parse_result();
    ~parse_result();

    parse_result(const parse_result &o);
    parse_result &operator=(const parse_result &o);
    parse_result(parse_result &&) noexcept;
    parse_result &operator=(parse_result &&) noexcept;

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
