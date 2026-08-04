module;

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

module silicon.config.json;

namespace silicon::config {

// ── Pimpl ─────────────────────────────────────────────────────────────────
struct JsonFileConfig::Impl {
    std::map<std::string, ConfigValue, std::less<>> entries_;
};

JsonFileConfig::JsonFileConfig(): impl_(std::make_unique<Impl>()) {}
JsonFileConfig::~JsonFileConfig() = default;

// ── JsonFileConfig methods ────────────────────────────────────────────────
bool JsonFileConfig::Load(const std::string &path, const fs::FileSystem &filesystem) {
    auto content = filesystem.Read(path);
    if(!content) return false;

    auto parsed = silicon::json::parse(content.value());
    if(parsed.is_discarded() || !parsed.is_object()) return false;

    for(auto it = parsed.begin(); it != parsed.end(); ++it) {
        impl_->entries_[it.key()] = ConfigValue{silicon::json::serialize(it.value())};
    }
    return true;
}

std::optional<ConfigValue> JsonFileConfig::Get(std::string_view key) const {
    auto it = impl_->entries_.find(key);
    if(it == impl_->entries_.end()) return std::nullopt;
    return it->second;
}

std::map<std::string, ConfigValue, std::less<>> JsonFileConfig::All() const {
    return impl_->entries_;
}

} // namespace silicon::config
