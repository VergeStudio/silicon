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

    struct P {
      public:
      std::string command;                      // 子命令名（若无则为空）
      std::map<std::string, std::string> flags; // --key value 或 --flag → "true"
      std::vector<std::string> positional;      // 位置参数
    };
    std::shared_ptr<P> m_p{std::make_shared<P>()};

  public:
    ParseResult() = default;
    ParseResult(const ParseResult &o): m_p(std::make_shared<P>(*o.m_p)) {}
    ParseResult &operator=(const ParseResult &o) {
        if (this != &o) { m_p = std::make_shared<P>(*o.m_p); }
        return *this;
    }
    ParseResult(ParseResult &&) noexcept = default;
    ParseResult &operator=(ParseResult &&) noexcept = default;

  public:
    std::string &command() { return m_p->command; }
    const std::string &command() const { return m_p->command; }
    std::map<std::string, std::string> &flags() { return m_p->flags; }
    const std::map<std::string, std::string> &flags() const { return m_p->flags; }
    std::vector<std::string> &positional() { return m_p->positional; }
    const std::vector<std::string> &positional() const { return m_p->positional; }

};

} // namespace silicon::cli
