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

    struct Impl {
      public:
      std::string command_;                      // 子命令名（若无则为空）
      std::map<std::string, std::string> flags_; // --key value 或 --flag → "true"
      std::vector<std::string> positional_;      // 位置参数
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    ParseResult() = default;
    ParseResult(const ParseResult &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    ParseResult &operator=(const ParseResult &o) {
        if (this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    ParseResult(ParseResult &&) noexcept = default;
    ParseResult &operator=(ParseResult &&) noexcept = default;

  public:
    std::string &Command() { return impl_->command_; }
    const std::string &Command() const { return impl_->command_; }
    std::map<std::string, std::string> &Flags() { return impl_->flags_; }
    const std::map<std::string, std::string> &Flags() const { return impl_->flags_; }
    std::vector<std::string> &Positional() { return impl_->positional_; }
    const std::vector<std::string> &Positional() const { return impl_->positional_; }

};

} // namespace silicon::cli
