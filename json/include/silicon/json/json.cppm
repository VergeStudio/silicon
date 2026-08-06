module;

// Local fork of nlohmann/json, adapted to silicon namespace.
// All types live in silicon::json_impl; the public type silicon::json::json
// is re-exported below.
import silicon.exception;
#include <silicon/json_impl/json.hpp>

export module silicon.json;

// ── Public type ───────────────────────────────────────────────────────────
// The json value type, re-exported from the forked implementation.
export namespace silicon::json {
using json = silicon::json_impl::json;
}
// Also export at module scope for convenience
export using silicon::json::json;

// ── Backward-compatible alias ─────────────────────────────────────────────
// Old code using silicon::json::JsonValue continues to work.
export namespace silicon::json {
using JsonValue = json;
}

// ── Convenience helpers ───────────────────────────────────────────────────
export namespace silicon::json {
inline json parse(std::string_view text) {
    return json::parse(text, nullptr, false);
}
inline std::string serialize(const json &value) {
    return value.dump();
}
} // namespace silicon::json
