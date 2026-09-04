

module;

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

module silicon.cli.parser.parse_result;

namespace silicon::cli {

struct parse_result::impl {
    std::string command_;
    std::map<std::string, std::string> flags_;
    std::vector<std::string> positional_;
};

parse_result::parse_result(): impl_(std::make_shared<impl>()) {}

parse_result::~parse_result() = default;

parse_result::parse_result(const parse_result &o): impl_(std::make_shared<impl>(*o.impl_)) {}

parse_result &parse_result::operator=(const parse_result &o) {
    if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
    return *this;
}

parse_result::parse_result(parse_result &&) noexcept = default;
parse_result &parse_result::operator=(parse_result &&) noexcept = default;

std::string &parse_result::command() { return impl_->command_; }

const std::string &parse_result::command() const { return impl_->command_; }

std::map<std::string, std::string> &parse_result::flags() { return impl_->flags_; }

const std::map<std::string, std::string> &parse_result::flags() const { return impl_->flags_; }

std::vector<std::string> &parse_result::positional() { return impl_->positional_; }

const std::vector<std::string> &parse_result::positional() const { return impl_->positional_; }

}
