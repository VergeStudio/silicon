module;
#include <memory>

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

export module silicon.cli.parser.parse_result;

#include "silicon/common.h"

export namespace silicon::cli {
/// 解析结果
struct CLI_API parse_result {

  public:
    parse_result();
    ~parse_result();

    parse_result(const parse_result &o);
    parse_result &operator=(const parse_result &o);
    parse_result(parse_result &&) noexcept;
    parse_result &operator=(parse_result &&) noexcept;

  public:
    std::string &command();
    const std::string &command() const;
    std::map<std::string, std::string> &flags();
    const std::map<std::string, std::string> &flags() const;
    std::vector<std::string> &positional();
    const std::vector<std::string> &positional() const;

  private:
    /// Implementation state, fully hidden in the implementation unit (src/parser/parse_result.cpp).
    struct impl;
    std::shared_ptr<impl> impl_;
};

} // namespace silicon::cli
