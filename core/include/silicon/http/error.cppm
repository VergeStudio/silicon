module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.http.error;

import silicon.error;


namespace silicon::http {

CORE_API std::atomic<const std::error_category *> http_error_category_instance{nullptr};

}

export namespace silicon::http {


enum class http_error {
    kRequestFailed = 1,
    kInvalidResponse,
    kTimeout,
    kUnknown,
};


class CORE_API http_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.http"; }
    std::string message(int ev) const override {
        switch(static_cast<http_error>(ev)) {
            case http_error::kRequestFailed: return "http request failed";
            case http_error::kInvalidResponse: return "invalid http response";
            case http_error::kTimeout: return "http request timed out";
            case http_error::kUnknown: return "unknown http error";
        }
        return "unknown http error";
    }
};


inline void inject_http_error_category(const std::error_category &cat) noexcept {
    http_error_category_instance.store(&cat, std::memory_order_release);
}


[[nodiscard]] inline const std::error_category &http_category() noexcept {
    const std::error_category *cat = http_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}


[[nodiscard]] inline std::error_code make_error_code(http_error e) noexcept {
    return {static_cast<int>(e), http_category()};
}

}
