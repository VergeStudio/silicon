module;

#include <expected>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

module silicon.cli.parser;

// 实现单元隐式导入自身接口单元（silicon.cli.parser），显式 import 自身属
// ill-formed：clang 报 "import of module 'X' appears within its own implementation"，
// MSVC 容忍但不标准。parse_result / cli_error 经接口的 export import 可见。
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

    bool positional_only = false; // 裸 '--' 之后全部为位置参数
    while(i < argc) {
        std::string_view arg(argv[i]);
        if(positional_only || arg.empty() || arg[0] != '-') {
            result.positional().emplace_back(argv[i]);
            ++i;
            continue;
        }

        const bool is_long = arg.size() >= 2 && arg[1] == '-';

        // 裸 '--'：分隔符，后续全部为位置参数（GNU 惯例）。
        if(is_long && arg.size() == 2) {
            positional_only = true;
            ++i;
            continue;
        }
        // 裸 '-'：畸形 flag。
        if(!is_long && arg.size() == 1) {
            return std::unexpected(make_error_code(cli_error::kInvalidValue));
        }

        const size_t name_start = is_long ? 2 : 1;
        std::string name(arg.substr(name_start));

        // 显式值语法：--name=value（仅长 flag 支持等号，最高优先级）。
        std::optional<std::string> inline_value;
        if(is_long) {
            if(auto eq = name.find('='); eq != std::string::npos) {
                inline_value = name.substr(eq + 1);
                name.erase(eq);
            }
        }
        if(name.empty()) {
            return std::unexpected(make_error_code(cli_error::kInvalidValue));
        }

        // 声明驱动：已登记的 flag 按 AddFlag 的 requires_value 决定是否消费值；
        // 未登记（宽松维度）保留词法回退——长 flag 后随非 '-' token 则带值。
        const auto it = impl_->flags.find(name);
        bool takes_value;
        if(it != impl_->flags.end()) {
            takes_value = it->second;
        } else {
            takes_value = is_long && !inline_value.has_value() && (i + 1 < argc) && (argv[i + 1][0] != '-');
        }

        if(inline_value.has_value()) {
            result.flags()[name] = std::move(*inline_value);
            seen.push_back({std::move(name), true});
            ++i;
        } else if(takes_value) {
            if(i + 1 >= argc) {
                return std::unexpected(make_error_code(cli_error::kMissingArgument));
            }
            result.flags()[name] = argv[i + 1];
            seen.push_back({std::move(name), true});
            i += 2;
        } else {
            result.flags()[name] = "true";
            seen.push_back({std::move(name), false});
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
