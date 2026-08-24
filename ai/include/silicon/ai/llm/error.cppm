module;

#include <string>
#include <system_error>

export module silicon.ai.llm.error;

// category 实现类前向声明：不导出，完整定义见 error.cpp（实现单元）。
class llm_category_impl;

export namespace silicon::ai::llm {

/// LLM 语义错误枚举（专属 category：silicon.ai）。
enum class llm_error {
    kProviderUnavailable = 1,
    kInvalidResponse,
    kToolNotFound,
    kTimeout,
    kUnknown,
};

/// 返回 llm_error 专属 error_category（name() == "silicon.ai"）。
/// 声明留在接口单元；定义必须放在实现单元 error.cpp —— 其函数体
/// `static llm_category_impl cat;` 需要 llm_category_impl 的完整定义。
[[nodiscard]] const std::error_category &llm_category() noexcept;

/// llm_error 枚举 → std::error_code（专属 category）。
/// 内联于接口单元：仅依赖 llm_category() 的声明，便于 ADL 与
/// std::error_code 的隐式转换（无需 import std 之外的额外可见性）。
[[nodiscard]] inline std::error_code make_error_code(llm_error e) noexcept {
    return {static_cast<int>(e), llm_category()};
}

} // namespace silicon::ai::llm
