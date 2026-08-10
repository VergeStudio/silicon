module;

// 标准库头必须置于全局模块片段：接口单元全局片段中的 #include 对实现单元不可达
#include <map>
#include <memory>
#include <string>
#include <vector>

module silicon.cli.parser.parse_result;

namespace silicon::cli {

struct parse_result::Impl {
    std::string command_;                      // 子命令名（若无则为空）
    std::map<std::string, std::string> flags_; // --key value 或 --flag → "true"
    std::vector<std::string> positional_;      // 位置参数
};

parse_result::parse_result(): impl_(std::make_shared<Impl>()) {}

parse_result::~parse_result() = default;

parse_result::parse_result(const parse_result &o): impl_(std::make_shared<Impl>(*o.impl_)) {}

parse_result &parse_result::operator=(const parse_result &o) {
    if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
    return *this;
}

parse_result::parse_result(parse_result &&) noexcept = default;

parse_result &parse_result::operator=(parse_result &&) noexcept = default;

std::string &parse_result::Command() { return impl_->command_; }

const std::string &parse_result::Command() const { return impl_->command_; }

std::map<std::string, std::string> &parse_result::Flags() { return impl_->flags_; }

const std::map<std::string, std::string> &parse_result::Flags() const { return impl_->flags_; }

std::vector<std::string> &parse_result::Positional() { return impl_->positional_; }

const std::vector<std::string> &parse_result::Positional() const { return impl_->positional_; }

} // namespace silicon::cli
