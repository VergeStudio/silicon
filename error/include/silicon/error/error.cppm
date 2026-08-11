/// @file error.cppm
/// @brief silicon 全项目统一错误码体系：std::error_code + 各模块错误枚举。
///
/// 设计约定（全项目 2026-08-11 起执行）：
///   - 所有可失败 API 统一返回 `std::expected<T, std::error_code>`；
///   - POSIX errno 类错误直接 `silicon::error::system_error(errno)`（generic_category）；
///   - 模块特有语义错误用本模块定义的 `xxx_error` 枚举 + make_error_code（专属 category）；
///   - 构造函数/析构/operator 重载等无法返回 expected 的点保留异常（见各模块说明）。
///   - 本模块无任何依赖（仅标准库），是所有模块的错误码单一来源。

module;

#include <string>
#include <system_error>

export module silicon.error;

export namespace silicon::error {

// ── 通用错误工厂 ─────────────────────────────────────────────────
/// 从 POSIX errno 值构造 std::error_code（generic_category）。
[[nodiscard]] inline std::error_code system_error(int errno_value) noexcept {
    return {errno_value, std::generic_category()};
}

/// 从 std::errc 枚举构造 std::error_code（generic_category）。
[[nodiscard]] inline std::error_code system_error(std::errc e) noexcept {
    return std::make_error_code(e);
}

// ── 模块错误枚举（API 统一返回 std::error_code，枚举为码值来源） ──────

// core::shared_library（动态库加载）
enum class library_error {
    kAlreadyLoaded = 1,
    kLoadFailed,
    kUnloadFailed,
    kSymbolNotFound,
    kInvalidHandle,
};

// fs（文件系统）
enum class fs_error {
    kOpenFailed = 1,
    kReadFailed,
    kWriteFailed,
    kNotExist,
    kPermissionDenied,
    kNotDirectory,
    kAlreadyExists,
    kUnknown,
};

// network（无 errno 对应的语义错误；errno 类直接用 system_error）
enum class network_error {
    kUdpNotBound = 1,
    kCancelled,
    kPollingError,
    kTimeout,
    kInvalidIpAddress,
    kUnknown,
};

// coroutine（channel / queue / ring_buffer 共享）
enum class channel_error {
    kClosed = 1,
    kTimeout,
    kCancelled,
};

// di（依赖注入容器）
enum class di_error {
    kDuplicateBinding = 1,
    kUnresolvedDependency,
    kCircularDependency,
    kInvalidType,
    kAlreadyInitialized,
    kUnknown,
};

// http
enum class http_error {
    kRequestFailed = 1,
    kInvalidResponse,
    kTimeout,
    kUnknown,
};

// llm
enum class llm_error {
    kProviderUnavailable = 1,
    kInvalidResponse,
    kToolNotFound,
    kTimeout,
    kUnknown,
};

// cli（命令行解析）
enum class cli_error {
    kParseFailed = 1,
    kUnknownOption,
    kMissingArgument,
    kInvalidValue,
};

// config
enum class config_error {
    kLoadFailed = 1,
    kParseFailed,
    kInvalidValue,
};

// event
enum class event_error {
    kInvalidStatus = 1,
};

// logger
enum class logger_error {
    kInitFailed = 1,
    kInvalidLevel,
};

// plugin
enum class plugin_error {
    kLoadFailed = 1,
    kUnloadFailed,
    kDuplicate,
    kNotFound,
};

// tui
enum class tui_error {
    kInitFailed = 1,
    kInvalidTerminal,
};

// ── category 访问器（实现单元 src/error.cpp 提供） ────────────────
[[nodiscard]] const std::error_category &library_category() noexcept;
[[nodiscard]] const std::error_category &fs_category() noexcept;
[[nodiscard]] const std::error_category &network_category() noexcept;
[[nodiscard]] const std::error_category &channel_category() noexcept;
[[nodiscard]] const std::error_category &di_category() noexcept;
[[nodiscard]] const std::error_category &http_category() noexcept;
[[nodiscard]] const std::error_category &llm_category() noexcept;
[[nodiscard]] const std::error_category &cli_category() noexcept;
[[nodiscard]] const std::error_category &config_category() noexcept;
[[nodiscard]] const std::error_category &event_category() noexcept;
[[nodiscard]] const std::error_category &logger_category() noexcept;
[[nodiscard]] const std::error_category &plugin_category() noexcept;
[[nodiscard]] const std::error_category &tui_category() noexcept;

// ── make_error_code（枚举 → std::error_code，显式转换） ────────────
[[nodiscard]] std::error_code make_error_code(library_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(fs_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(network_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(channel_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(di_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(http_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(llm_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(cli_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(config_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(event_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(logger_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(plugin_error e) noexcept;
[[nodiscard]] std::error_code make_error_code(tui_error e) noexcept;

} // namespace silicon::error
