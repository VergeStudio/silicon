module;

#include <expected>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

module silicon.cli.parser;

import silicon.cli.parser;
import silicon.cli.error;

namespace silicon::cli {

struct Parser::impl {
    std::set<std::string> subcommands;
    std::map<std::string, bool> flags; // name -> requires_value
};

Parser::Parser() : impl_(std::make_unique<impl>()) {}
Parser::~Parser() = default;

void Parser::AddSubcommand(std::string name) {
    impl_->subcommands.insert(std::move(name));
}

void Parser::AddFlag(std::string name, bool requires_value) {
    impl_->flags.emplace(std::move(name), requires_value);
}

std::expected<parse_result, std::error_code> Parser::Parse(int argc, const char *const *argv) const {
    parse_result result;
    if(argc <= 0) return result;

    int i = 1;
    // 子命令：首个不以 '-' 开头的 token
    if(i < argc && argv[i][0] != '-') {
        result.command() = argv[i];
        ++i;
    }

    struct flag_seen {
        std::string name;
        bool has_value;
    };
    std::vector<flag_seen> seen;

    while(i < argc) {
        std::string_view arg(argv[i]);
        const bool looks_like_flag = !arg.empty() && arg[0] == '-';
        if(looks_like_flag) {
            const bool is_long = arg.size() >= 2 && arg[1] == '-';
            const size_t name_start = is_long ? 2 : 1;
            std::string name(arg.substr(name_start));
            if(name.empty()) {
                // 裸 '-' 或 '--'：畸形 flag（非法 flag）
                return std::unexpected(make_error_code(cli_error::kInvalidValue));
            }
            // 短 flag（-v）语义为布尔开关；长 flag（--name）默认携带值
            // （除非后随 token 以 '-' 开头）。已登记的 requires_value=false
            // 长 flag 亦可在 token 前止步——此处仅按词法区分，校验由
            // 下方 seen 对照 flags_ 完成。
            const bool next_is_value = is_long && (i + 1 < argc) && (argv[i + 1][0] != '-');
            if(next_is_value) {
                result.flags()[name] = argv[i + 1];
                seen.push_back({std::move(name), true});
                i += 2;
            } else {
                result.flags()[name] = "true";
                seen.push_back({std::move(name), false});
                ++i;
            }
        } else {
            result.positional().emplace_back(argv[i]);
            ++i;
        }
    }

    // ── 校验：仅对声明过的维度生效，未声明则宽松通过 ──
    if(!impl_->subcommands.empty() && !result.command().empty()
       && impl_->subcommands.find(result.command()) == impl_->subcommands.end()) {
        return std::unexpected(make_error_code(cli_error::kUnknownSubcommand));
    }
    if(!impl_->flags.empty()) {
        for(const auto &f: seen) {
            auto it = impl_->flags.find(f.name);
            if(it == impl_->flags.end()) {
                return std::unexpected(make_error_code(cli_error::kUnknownOption));
            }
            if(it->second && !f.has_value) {
                return std::unexpected(make_error_code(cli_error::kMissingArgument));
            }
        }
    }

    return result;
}

} // namespace silicon::cli
