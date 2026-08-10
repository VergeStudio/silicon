module;
#include <memory>

#include <mutex>
#include <string>

module silicon.library;

import silicon.exception;

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

void *shared_library::get_symbol(const std::string &symbol_name) {
    void *result = find_symbol(symbol_name);
    if(result != nullptr) {
        return result;
    }

    throw silicon::exception::runtime_error("[shared_library::get_symbol]: can't find symbol ", symbol_name);
}

bool shared_library::has_symbol(const std::string &symbol_name) {
    return find_symbol(symbol_name) != nullptr;
}

std::string shared_library::get_os_name(const std::string &name) {
    return prefix() + name + suffix();
}
} // namespace silicon::library

// module silicon.library;
// module;
