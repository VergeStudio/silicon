module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>

export module silicon.cli.error;

import silicon.error;

export namespace silicon::cli {

CORE_API std::atomic<const std::error_category *> cli_error_category_instance{nullptr};

}

export namespace silicon::cli {

enum class cli_error {
    kParseFailed = 1,
    kUnknownOption,
    kMissingArgument,
    kInvalidValue,
    kUnknownSubcommand,
    kUnknown,
};

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

inline void inject_cli_error_category(const std::error_category &cat) noexcept {
    cli_error_category_instance.store(&cat, std::memory_order_release);
}

[[nodiscard]] inline const std::error_category &cli_category() noexcept {
    const std::error_category *cat = cli_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

[[nodiscard]] inline std::error_code make_error_code(cli_error e) noexcept {
    return {static_cast<int>(e), cli_category()};
}

}
