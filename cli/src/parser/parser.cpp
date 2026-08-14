module;

#include <expected>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

module silicon.cli.parser;

import silicon.cli.parser;
import silicon.cli.error;

namespace silicon::cli {

void Parser::AddSubcommand(std::string name) {
    subcommands_.insert(std::move(name));
}

void Parser::AddFlag(std::string name, bool requires_value) {
    flags_.emplace(std::move(name), requires_value);
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
            const size_t name_start = (arg.size() >= 2 && arg[1] == '-') ? 2 : 1;
            std::string name(arg.substr(name_start));
            if(name.empty()) {
                // 裸 '-' 或 '--'：畸形 flag（非法 flag）
                return std::unexpected(make_error_code(cli_error::kInvalidValue));
            }
            const bool next_is_value = (i + 1 < argc) && (argv[i + 1][0] != '-');
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
    if(!subcommands_.empty() && !result.command().empty()
       && subcommands_.find(result.command()) == subcommands_.end()) {
        return std::unexpected(make_error_code(cli_error::kUnknownSubcommand));
    }
    if(!flags_.empty()) {
        for(const auto &f: seen) {
            auto it = flags_.find(f.name);
            if(it == flags_.end()) {
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
