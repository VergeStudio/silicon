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

JsonFileConfig::JsonFileConfig(): m_impl(std::make_unique<Impl>()) {}
JsonFileConfig::~JsonFileConfig() = default;

// ── JsonFileConfig methods ────────────────────────────────────────────────
bool JsonFileConfig::load(const std::string &path, const fs::IFileSystem &filesystem) {
    auto content = filesystem.read(path);
    if(!content) return false;

    auto parsed = silicon::json::parse(content.value());
    if(parsed.is_discarded() || !parsed.is_object()) return false;

    for(auto it = parsed.begin(); it != parsed.end(); ++it) {
        m_impl->entries_[it.key()] = ConfigValue{silicon::json::serialize(it.value())};
    }
    return true;
}

std::optional<ConfigValue> JsonFileConfig::get(std::string_view key) const {
    auto it = m_impl->entries_.find(key);
    if(it == m_impl->entries_.end()) return std::nullopt;
    return it->second;
}

std::map<std::string, ConfigValue, std::less<>> JsonFileConfig::all() const {
    return m_impl->entries_;
}

} // namespace silicon::config
