module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include "silicon/ai/common.h"
export module silicon.ai.llm.error;

import silicon.error;



namespace silicon::ai::llm {




AI_API std::atomic<const std::error_category *> llm_error_category_instance{nullptr};

}

export namespace silicon::ai::llm {


enum class llm_error {
    kProviderUnavailable = 1,
    kInvalidResponse,
    kToolNotFound,
    kTimeout,
    kUnknown,
};






class AI_API llm_category_impl final: public std::error_category {
    const char *name() const noexcept override { return "silicon.ai.llm"; }
    std::string message(int ev) const override {
        switch(static_cast<llm_error>(ev)) {
            case llm_error::kProviderUnavailable:
                return "llm provider unavailable";
            case llm_error::kInvalidResponse:
                return "invalid llm response";
            case llm_error::kToolNotFound:
                return "tool not found";
            case llm_error::kTimeout:
                return "llm request timed out";
            case llm_error::kUnknown:
                return "unknown llm error";
        }
        return "unknown llm error";
    }
};



inline void inject_llm_error_category(const std::error_category &cat) noexcept {
    llm_error_category_instance.store(&cat, std::memory_order_release);
}





[[nodiscard]] inline const std::error_category &llm_error_category() noexcept {
    const std::error_category *cat = llm_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}


[[nodiscard]] inline std::error_code make_error_code(llm_error e) noexcept {
    return {static_cast<int>(e), llm_error_category()};
}

}
