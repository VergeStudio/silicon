module;
#include <memory>

#include <expected>
#include <mutex>
#include <string>
#include <system_error>

module silicon.library;

#include "shared_library_impl.hpp"

namespace silicon::library {

// ── 平台无关的公共成员（原在各平台实现单元重复定义，统一收拢于此） ──────
shared_library::shared_library() : impl_(std::make_unique<Impl>()) {
}

shared_library::~shared_library() = default;

bool shared_library::is_loaded() const {
    return impl_->handle_ != nullptr;
}

const std::string &shared_library::get_path() const {
    return impl_->path_;
}

auto shared_library::get_symbol(const std::string &symbol_name) -> std::expected<void *, std::error_code> {
    void *result = find_symbol(symbol_name);
    if(result != nullptr) {
        return result;
    }
    return std::unexpected(make_error_code(library_error::kSymbolNotFound));
}

bool shared_library::has_symbol(const std::string &symbol_name) {
    return find_symbol(symbol_name) != nullptr;
}

std::string shared_library::get_os_name(const std::string &name) {
    return prefix() + name + suffix();
}

// ── library_error category 与 make_error_code ──────────────────
namespace {
class library_error_category final : public std::error_category {
  public:
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
} // namespace

const std::error_category &library_category() noexcept {
    static const library_error_category cat{};
    return cat;
}

std::error_code make_error_code(library_error e) noexcept {
    return {static_cast<int>(e), library_category()};
}

} // namespace silicon::library
