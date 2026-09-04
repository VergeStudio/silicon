module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>



#include <silicon/common.h>

export module silicon.library.error;

import silicon.error;




export namespace silicon::library {

CORE_API std::atomic<const std::error_category *> library_error_category_instance{nullptr};

}

export namespace silicon::library {


enum class library_error {
    kAlreadyLoaded = 1,
    kLoadFailed,
    kUnloadFailed,
    kSymbolNotFound,
    kInvalidHandle,
};



class CORE_API library_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.library"; }
    std::string message(int ev) const override {
        switch(static_cast<library_error>(ev)) {
            case library_error::kAlreadyLoaded: return "library already loaded";
            case library_error::kLoadFailed: return "failed to load shared library";
            case library_error::kUnloadFailed: return "failed to unload shared library";
            case library_error::kSymbolNotFound: return "symbol not found in shared library";
            case library_error::kInvalidHandle: return "invalid shared library handle";
        }
        return "unknown library error";
    }
};


inline void inject_library_error_category(const std::error_category &cat) noexcept {
    library_error_category_instance.store(&cat, std::memory_order_release);
}


[[nodiscard]] inline const std::error_category &library_category() noexcept {
    const std::error_category *cat = library_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}


[[nodiscard]] inline std::error_code make_error_code(library_error e) noexcept {
    return {static_cast<int>(e), library_category()};
}

}






namespace {
    const silicon::library::library_category_impl s_default_library_category{};
    const bool s_library_category_registered = [] {
        silicon::library::inject_library_error_category(s_default_library_category);
        return true;
    }();
}
