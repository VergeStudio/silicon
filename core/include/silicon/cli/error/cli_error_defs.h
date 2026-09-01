#pragma once

#include <atomic>
#include <string>
#include <system_error>

namespace silicon::cli {

/// cli_error / make_error_code / cli_category / cli_category_impl / cli_error_category_instance
/// 为 header-only（global module 实体），原因与 silicon::time::system_clock、
/// silicon::http::http_response 完全一致（见各头内 mangling 说明）：
/// 1. MSVC 对命名模块实体的修饰名追加模块标签（`::<!silicon.cli.error>`），clang 不
///    生成该标签 → 任何「MSVC 编译的定义 + clang 引用」的跨工具链链接均无法解析；
/// 2. MSVC IFC 不含成员函数体，clang 导入模块后对成员函数发强引用，无法本地内联展开。
///
/// 本头把 cli 错误域抽为纯文本头，由消费方（parser_types.h / 测试 / 组合根）直接
/// #include，使每个消费 TU 本地发射（weak）符号，彻底规避跨 DLL / 跨工具链符号解析。
///
/// 单一实例契约：原 cli_error_category_instance 为 CORE_API 模块导出实体（进程唯一、
/// 跨 DLL 共享）。折为 header-only 后变为 inline 全局（每二进制一份）；cli 错误域仅被
/// header-only 的 silicon::cli::parser 消费，而 parser 与组合根同处消费二进制，单一实例
/// 契约在各自二进制内仍满足，故无跨 DLL 重复单例问题。
enum class cli_error {
    kParseFailed = 1,
    kUnknownOption,
    kMissingArgument,
    kInvalidValue,
    kUnknownSubcommand,
    kUnknown,
};

/// 具名类取代匿名类局部静态：MSVC 模块构建对匿名派生类的 vtable 处理有缺陷
/// （name()/message() 虚调用会 SIGSEGV）。原为 CORE_API 模块导出实体，现折为
/// header-only 全局实体（见文件头注释）。
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
inline std::atomic<const std::error_category *> cli_error_category_instance{nullptr};

/// 组合根注入全局唯一 category 实例（必须在任何 make_error_code 调用之前完成）。
/// 注入后模块独占该句柄；std::error_category 设计上进程期常驻，析构为保护，真实
/// 生命周期由组合根持有，模块侧不释放。
inline void inject_cli_error_category(const std::error_category &cat) noexcept {
    cli_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 cli_error 专属 error_category。
/// DI 是唯一来源：组合根必须先注入；未注入即使用属组合根接线错误，直接终止。
/// 不提供模块内 fallback 单例——否则会破坏 std::error_category「全局唯一地址」契约，
/// 并在跨 DLL / 多二进制场景下重现重复单例问题。
[[nodiscard]] inline const std::error_category &cli_category() noexcept {
    const std::error_category *cat = cli_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 cli_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(cli_error e) noexcept {
    return {static_cast<int>(e), cli_category()};
}

} // namespace silicon::cli
