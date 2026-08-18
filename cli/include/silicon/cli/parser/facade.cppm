module;

#include <expected>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <tuple>
// proxy dispatch 宏头：宏不随 C++20 模块导出，必须在全局模块片段文本包含
#include <silicon/proxy/proxy_macros.h>
#include "silicon/cli/common.h"
export module silicon.cli.parser;



export import silicon.cli.parser.parse_result;
export import silicon.cli.error;

import silicon.proxy;

export namespace silicon::cli {

// ── 类型擦除门面（silicon.proxy）─────────
//
// 目标类型无需继承任何基类，只要拥有 `Parse` 成员即自动满足门面（鸭子类型）。
// 既有的具体类 Parser 直接接入，不再耦合任何继承体系。

PRO_DEF_MEM_DISPATCH(MemParserParse, Parse);

/// 解析器门面：满足 `std::expected<parse_result, std::error_code>
/// Parse(int, const char *const *) const`。
struct parser_facade
    : silicon::proxy::facade_builder                                                                  //
      ::add_convention<MemParserParse, std::expected<parse_result, std::error_code>(int, const char *const *) const> //
      ::build {};

/// 拥有所有权的类型擦除句柄（值语义）。
using parser_proxy = silicon::proxy::proxy<parser_facade>;

/// 非拥有观察视图，等价于裸指针但不要求继承。
using parser_view = silicon::proxy::proxy_view<parser_facade>;

/// 就地构造任意满足门面的目标类型并擦除为 parser_proxy；句柄按值持有。
template<class T, class... Args>
[[nodiscard]] parser_proxy make_parser(Args &&...args) {
    return silicon::proxy::make_proxy<parser_facade, T>(std::forward<Args>(args)...);
}

/// 默认 CLI 解析器（鸭子类型满足 parser_facade，零抽象基类耦合）。
CLI_API class Parser {
  public:
    /// 声明合法子命令。Parse 会校验首个位置参数是否落在已知子命令集合内，
    /// 未登记任何子命令时该维度不做校验（宽松通过）。
    void AddSubcommand(std::string name);
    /// 声明合法命名标志。requires_value=true 时该 flag 必须携带值，否则返回 kMissingArgument。
    /// 未登记任何 flag 时该维度不做校验（宽松通过）。
    void AddFlag(std::string name, bool requires_value = false);

    /// 解析 argv；成功返回 parse_result，失败返回 cli_error 对应的 error_code。
    std::expected<parse_result, std::error_code> Parse(int argc, const char *const *argv) const;

  private:
    std::set<std::string> subcommands_;
    std::map<std::string, bool> flags_; // name -> requires_value
};

} // namespace silicon::cli
