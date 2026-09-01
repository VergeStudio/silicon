module;

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <tuple>
// proxy dispatch 宏头：宏不随 C++20 模块导出，必须在全局模块片段文本包含
#include <silicon/proxy/proxy_macros.h>
#include "silicon/common.h"
// parser / parse_result 为 header-only（global module 实体），见 parser_types.h 内
// mangling 说明；其内部已包含 cli_error_defs.h（make_error_code 同为 header-only）。
#include <silicon/cli/parser/parser_types.h>
export module silicon.cli.parser;



export import silicon.cli.parser.parse_result;

import silicon.proxy;

export namespace silicon::cli {

// ── 类型擦除门面（silicon.proxy）─────────
//
// 目标类型无需继承任何基类，只要拥有 `parse` 成员即自动满足门面（鸭子类型）。
// 既有的具体类 parser 直接接入，不再耦合任何继承体系。

PRO_DEF_MEM_DISPATCH(MemParserParse, parse);

/// 解析器门面：满足 `std::expected<parse_result, std::error_code>
/// parse(int, const char *const *) const`。
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

// 注：parser / parse_result 已折为 header-only（见 parser_types.h），不再作为模块导出实体；
// 消费方（如 siliconbuddy.cli）直接 #include <silicon/cli/parser/parser_types.h> 取用。

} // namespace silicon::cli
