module;

#include <expected>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

module silicon.config.json;

import silicon.config.error;

namespace silicon::config {

// ── Pimpl ─────────────────────────────────────────────────────────────────
struct JsonFileConfig::impl {
    std::map<std::string, config_value, std::less<>> entries_;
};

JsonFileConfig::JsonFileConfig(): impl_(std::make_unique<impl>()) {}
JsonFileConfig::~JsonFileConfig() = default;

// ── JsonFileConfig methods ────────────────────────────────────────────────
auto JsonFileConfig::Load(const std::string &path, const fs::file_system_view &filesystem) -> result<void> {
    auto content = filesystem->read(path);
    if(!content) return std::unexpected(make_error_code(config_error::kLoadFailed));

    auto parsed = silicon::json::parse(content.value());
    if(parsed.is_discarded() || !parsed.is_object()) return std::unexpected(make_error_code(config_error::kParseFailed));

    for(auto it = parsed.begin(); it != parsed.end(); ++it) {
        impl_->entries_[it.key()] = config_value{silicon::json::serialize(it.value())};
    }
    return {};
}

std::optional<config_value> JsonFileConfig::Get(std::string_view key) const {
    auto it = impl_->entries_.find(key);
    if(it == impl_->entries_.end()) return std::nullopt;
    return it->second;
}

std::map<std::string, config_value, std::less<>> JsonFileConfig::All() const {
    return impl_->entries_;
}

} // namespace silicon::config
