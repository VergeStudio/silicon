/// @file error.cppm
/// @brief 统一错误返回类型：std::expected<T, std::error_code> 的集中别名。
/// @usage
///   import silicon.error;
///   silicon::error::result<T>  // == std::expected<T, std::error_code>
///
/// 全项目可失败 API 的统一返回形态集中定义于此，各业务模块不再各自重复声明，
/// 统一经 `import silicon.error;` 复用本别名（见各模块 facade 中的转发别名）。

module;

#include <expected>
#include <system_error>

export module silicon.error;

export namespace silicon::error {

/// 统一错误返回类型：全项目可失败 API 的统一返回形态。
/// 错误码统一为 std::error_code（可由各模块专属 error_category 构造）。
template<typename T>
using result = std::expected<T, std::error_code>;

} // namespace silicon::error
