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
[[nodiscard]] inline const std::error_category &llm_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.ai"; }
        std::string message(int ev) const override {
            switch(static_cast<llm_error>(ev)) {
                case llm_error::kProviderUnavailable: return "llm provider unavailable";
                case llm_error::kInvalidResponse: return "invalid llm response";
                case llm_error::kToolNotFound: return "tool not found";
                case llm_error::kTimeout: return "llm request timed out";
                case llm_error::kUnknown: return "unknown llm error";
            }
            return "unknown llm error";
        }
    } cat;
    return cat;
}

/// llm_error 枚举 → std::error_code（专属 category）。
[[nodiscard]] inline std::error_code make_error_code(llm_error e) noexcept {
    return {static_cast<int>(e), llm_category()};
}

} // namespace silicon::ai::llm
