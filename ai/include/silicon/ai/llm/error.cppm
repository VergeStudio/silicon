module;

#include <string>
#include <system_error>

export module silicon.ai.llm.error;

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
[[nodiscard]] const std::error_category &llm_category() noexcept;

/// llm_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] std::error_code make_error_code(llm_error e) noexcept;

} // namespace silicon::ai::llm
