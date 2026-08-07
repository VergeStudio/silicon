module;

// 标准库头必须置于全局模块片段：接口单元全局片段中的 #include 对实现单元不可达
#include <map>
#include <memory>
#include <string>
#include <vector>

module silicon.cli.parser.parse_result;

namespace silicon::cli {

struct ParseResult::Impl {
    std::string command_;                      // 子命令名（若无则为空）
    std::map<std::string, std::string> flags_; // --key value 或 --flag → "true"
    std::vector<std::string> positional_;      // 位置参数
};

ParseResult::ParseResult(): impl_(std::make_shared<Impl>()) {}

ParseResult::~ParseResult() = default;

ParseResult::ParseResult(const ParseResult &o): impl_(std::make_shared<Impl>(*o.impl_)) {}

ParseResult &ParseResult::operator=(const ParseResult &o) {
    if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
    return *this;
}

ParseResult::ParseResult(ParseResult &&) noexcept = default;

ParseResult &ParseResult::operator=(ParseResult &&) noexcept = default;

std::string &ParseResult::Command() { return impl_->command_; }

const std::string &ParseResult::Command() const { return impl_->command_; }

std::map<std::string, std::string> &ParseResult::Flags() { return impl_->flags_; }

const std::map<std::string, std::string> &ParseResult::Flags() const { return impl_->flags_; }

std::vector<std::string> &ParseResult::Positional() { return impl_->positional_; }

const std::vector<std::string> &ParseResult::Positional() const { return impl_->positional_; }

} // namespace silicon::cli
