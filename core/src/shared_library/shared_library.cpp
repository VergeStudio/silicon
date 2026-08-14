module;
#include <memory>

#include <expected>
#include <mutex>
#include <string>
#include <system_error>

module silicon.library;
import silicon.library.error;

#include "shared_library_impl.hpp"

namespace silicon::library {

// ── 平台无关的公共成员（原在各平台实现单元重复定义，统一收拢于此） ──────
shared_library::shared_library() : impl_(std::make_unique<impl>()) {
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


} // namespace silicon::library
