module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

// CORE_API 宏（dllexport/dlimport 闸门）：cli_category_impl 的 vtable 须跨 DLL 导出，
// 否则消费方（测试/应用）构造 category / 内联 make_error_code 时无法解析其虚函数槽。
#include <silicon/common.h>

export module silicon.cli.error;

import silicon.error;

// ---- 跨 DLL 共享的 DI 句柄：须显式 dllexport（CORE_API），否则被内联进消费方的
// inject_cli_error_category / cli_category / make_error_code 无法解析其实例地址（LNK2001）。
// 模块链接的 export 变量在直接编译进 DLL 时不会自动导出到导入库，须 *API 强标。----
export namespace silicon::cli {

CORE_API std::atomic<const std::error_category *> cli_error_category_instance{nullptr};

} // namespace silicon::cli

export namespace silicon::cli {

/// cli 模块专属错误码枚举（参数解析失败）。
enum class cli_error {
    kParseFailed = 1,
    kUnknownOption,
    kMissingArgument,
    kInvalidValue,
    kUnknownSubcommand,
    kUnknown,
};

// 具名类取代匿名类局部静态（MSVC 模块构建对匿名派生类的 vtable 处理有缺陷，
// name()/message() 虚调用会 SIGSEGV）；由组合根构造并注入。
// CORE_API 强制导出其 vtable（name/message 虚函数槽），否则跨 DLL 消费方解析失败。
class CORE_API cli_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.cli"; }
    std::string message(int ev) const override {
        switch (static_cast<cli_error>(ev)) {
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

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
/// 注入后模块独占该句柄；std::error_category 设计上进程期常驻，故不释放。
inline void inject_cli_error_category(const std::error_category &cat) noexcept {
    cli_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 cli_error 专属 error_category。
/// DI 是唯一来源：组合根必须先注入；未注入即使用属组合根接线错误，直接终止。
[[nodiscard]] inline const std::error_category &cli_category() noexcept {
    const std::error_category *cat = cli_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// cli_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(cli_error e) noexcept {
    return {static_cast<int>(e), cli_category()};
}

} // namespace silicon::cli
