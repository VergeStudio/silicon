module;

#include <exception>
#include <memory>
#include <string>
#include <system_error>

export module silicon.cli.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::cli {

// 用 silicon.error 提供的 no-op 删除器承载「模块独占 category 句柄」语义而不实际 delete
// （std::error_category 析构为保护、进程期常驻；真实生命周期由组合根持有）。
inline std::unique_ptr<const std::error_category, silicon::error::category_deleter> cli_error_category_instance;

} // namespace silicon::cli

export namespace silicon::cli {

/// cli 模块专属错误码枚举（命令行解析语义错误）。
enum class cli_error {
    kParseFailed = 1,
    kUnknownOption,
    kMissingArgument,
    kInvalidValue,
    kUnknownSubcommand,
    kUnknown,
};

// 具名类取代匿名类局部静态：MSVC 模块构建对匿名派生类的 vtable 处理有缺陷
// （name()/message() 虚调用会 SIGSEGV）。该实现类由组合根实例化并经 inject_cli_error_category 注入，
// 模块自身不再持有单例；下层统一经 cli_category() 取到同一实例，跨模块/跨 DLL 安全。
class cli_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.cli"; }
    std::string message(int ev) const override {
        switch(static_cast<cli_error>(ev)) {
            case cli_error::kParseFailed: return "cli parse failed";
            case cli_error::kUnknownOption: return "unknown option";
            case cli_error::kMissingArgument: return "option requires an argument";
            case cli_error::kInvalidValue: return "invalid argument value";
            case cli_error::kUnknownSubcommand: return "unknown subcommand";
            case cli_error::kUnknown: return "unknown cli error";
        }
        return "unknown cli error";
    }
};

/// 组合根注入全局唯一 category 实例（必须在任何 make_error_code 调用之前完成）。
/// 注入后模块独占该句柄；std::error_category 设计上进程期常驻，故不释放。
inline void inject_cli_error_category(const std::error_category &cat) noexcept {
    cli_error_category_instance.reset(&cat);
}

/// 返回 cli_error 专属 error_category。
/// DI 是唯一来源：组合根必须先注入；未注入即使用属组合根接线错误，直接终止。
/// 不提供模块内 fallback 单例——否则会破坏 std::error_category「全局唯一地址」契约，
/// 并在跨 DLL / 多二进制场景下重现重复单例问题。
[[nodiscard]] inline const std::error_category &cli_category() noexcept {
    if (!cli_error_category_instance) {
        std::terminate();
    }
    return *cli_error_category_instance;
}

/// 将 cli_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(cli_error e) noexcept {
    return {static_cast<int>(e), cli_category()};
}

} // namespace silicon::cli
