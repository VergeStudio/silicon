module;

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <silicon/common.h>

export module silicon.cli.parser.parse_result;

export namespace silicon::cli {

/// argv 解析结果：子命令名 + 命名标志表 + 位置参数表。
///
/// PIMPL：实现状态完全隐藏于实现单元
/// （core/src/cli/parser/parse_result.cpp）。全部方法体 out-of-line 定义。
/// 类级 CORE_API 导出跨 DLL 符号；智能指针成员不触发 C4251（实测）。
class CORE_API parse_result {
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

} // namespace silicon::cli
