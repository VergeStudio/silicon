#pragma once

#include <expected>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <silicon/cli/error/cli_error_defs.h>

namespace silicon::cli {

/// parser / parse_result 为 header-only（global module 实体），与
/// silicon::time::system_clock、silicon::http::http_response / http_request 同款范式
/// （见各头内 mangling 说明）：
/// 1. MSVC 对命名模块实体的修饰名追加模块标签（`::<!silicon.cli.parser>`），clang 不
///    生成该标签 → 任何「MSVC 编译的定义 + clang 引用」的跨工具链链接均无法解析；
/// 2. MSVC IFC 不含成员函数体，clang 导入模块后对成员函数发强引用，无法本地内联展开。
///
/// 把这两个类型抽到纯文本头、在 silicon.cli.parser 模块接口的全局模块片段包含、并由
/// 消费方（如 siliconbuddy.cli）直接 #include 本头，可让每个消费 TU 本地发射（weak）
/// 符号，彻底规避跨 DLL / 跨工具链符号解析。
///
/// 注意：parser::parse 依赖 make_error_code(cli_error)（见 cli_error_defs.h，同为
/// header-only 全局实体），故本头包含 cli_error_defs.h——否则 parser::parse 的函数体
/// 在全局模块片段无法引用模块导出实体。
///
/// 顺序：parse_result 必须先于 parser 完整定义，因为 parser::parse 的返回类型按值携带
/// parse_result，需要其完整类型。

struct parse_result {

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

class parser {
  public:
    parser();
    ~parser();

    /// 声明合法子命令。parse 会校验首个位置参数是否落在已知子命令集合内，
    /// 未登记任何子命令时该维度不做校验（宽松通过）。
    void add_subcommand(std::string);
    /// 声明合法命名标志。requires_value=true 时该 flag 必须携带值，否则返回 kMissingArgument。
    /// 未登记任何 flag 时该维度不做校验（宽松通过）。
    void add_flag(std::string, bool = false);

    /// 解析 argv；成功返回 parse_result，失败返回 cli_error 对应的 error_code。
    std::expected<parse_result, std::error_code> parse(int, const char *const *) const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

// ── parse_result::impl ───────────────────────────────────────────
struct parse_result::impl {
    std::string command_;                      // 子命令名（若无则为空）
    std::map<std::string, std::string> flags_; // --key value 或 --flag → "true"
    std::vector<std::string> positional_;      // 位置参数
};

inline parse_result::parse_result(): impl_(std::make_shared<impl>()) {}
inline parse_result::~parse_result() = default;
inline parse_result::parse_result(const parse_result &o): impl_(std::make_shared<impl>(*o.impl_)) {}
inline parse_result &parse_result::operator=(const parse_result &o) {
    if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
    return *this;
}
inline parse_result::parse_result(parse_result &&) noexcept = default;
inline parse_result &parse_result::operator=(parse_result &&) noexcept = default;

inline std::string &parse_result::command() { return impl_->command_; }
inline const std::string &parse_result::command() const { return impl_->command_; }
inline std::map<std::string, std::string> &parse_result::flags() { return impl_->flags_; }
inline const std::map<std::string, std::string> &parse_result::flags() const { return impl_->flags_; }
inline std::vector<std::string> &parse_result::positional() { return impl_->positional_; }
inline const std::vector<std::string> &parse_result::positional() const { return impl_->positional_; }

// ── parser::impl ─────────────────────────────────────────────────
struct parser::impl {
    std::set<std::string> subcommands;
    std::map<std::string, bool> flags; // name -> requires_value
};

inline parser::parser() : impl_(std::make_unique<impl>()) {}
inline parser::~parser() = default;

inline void parser::add_subcommand(std::string name) {
    impl_->subcommands.insert(std::move(name));
}

inline void parser::add_flag(std::string name, bool requires_value) {
    impl_->flags.emplace(std::move(name), requires_value);
}

inline std::expected<parse_result, std::error_code> parser::parse(int argc, const char *const *argv) const {
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

        // 声明驱动：已登记的 flag 按 add_flag 的 requires_value 决定是否消费值；
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
