module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>
#include <silicon/common.h>

export module silicon.di.error;

import silicon.error;

export namespace silicon::di {

CORE_API std::atomic<const std::error_category *> di_error_category_instance{nullptr};

enum class di_error {
    kDuplicateBinding = 1,
    kUnresolvedDependency,
    kCircularDependency,
    kInvalidType,
    kAlreadyInitialized,
    kTypeNotFound,
    kTypeAmbiguous,
    kTypeNotConvertible,
    kTypeRecursion,
    kTypeAlreadyRegistered,
    kTypeIndexAlreadyRegistered,
    kCollectionTypeNotFound,
    kIndexOutOfRange,
    kUnknown,
};

class CORE_API di_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.di"; }
    std::string message(int ev) const override {
        switch(static_cast<di_error>(ev)) {
            case di_error::kDuplicateBinding: return "duplicate binding";
            case di_error::kUnresolvedDependency: return "unresolved dependency";
            case di_error::kCircularDependency: return "circular dependency detected";
            case di_error::kInvalidType: return "invalid type";
            case di_error::kAlreadyInitialized: return "already initialized";
            case di_error::kTypeNotFound: return "requested type not found in container";
            case di_error::kTypeAmbiguous: return "requested type resolves ambiguously";
            case di_error::kTypeNotConvertible: return "registered type is not convertible to requested type";
            case di_error::kTypeRecursion: return "recursive type resolution detected";
            case di_error::kTypeAlreadyRegistered: return "type already registered";
            case di_error::kTypeIndexAlreadyRegistered: return "type index already registered";
            case di_error::kCollectionTypeNotFound: return "collection element type not found";
            case di_error::kIndexOutOfRange: return "type index out of range";
            case di_error::kUnknown: return "unknown di error";
        }
        return "unknown di error";
    }
};

inline void inject_di_error_category(const std::error_category &cat) noexcept {
    di_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &di_category() noexcept {
    const std::error_category *cat = di_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(di_error e) noexcept {
    return {static_cast<int>(e), di_category()};
}

}

namespace {
    const silicon::di::di_category_impl s_default_di_category{};
    const bool s_di_category_registered = [] {
        silicon::di::inject_di_error_category(s_default_di_category);
        return true;
    }();
}
