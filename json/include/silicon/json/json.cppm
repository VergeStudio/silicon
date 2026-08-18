module;

// Standard headers needed by the convenience helpers below (string_view is
// also used by json.hpp but header-unit import only re-exports what json.hpp
// itself declares; std headers are not reachable through it).
#include <string>
#include <string_view>

export module silicon.json;

// Header-unit import: json.hpp is compiled once as a header unit, so all of
// its external-linkage entities (incl. the friend class templates that
// basic_json references) become exported and reachable in consumers. A plain
// GMF #include would keep them module-private and MSVC would fail with C2039
// when a consumer instantiates basic_json (re-resolution of friend decls).
export import "silicon/json_impl/json.hpp";

// Public type, re-exported from the forked implementation.
export namespace silicon::json {
using json = silicon::json_impl::json;
}
// Also export at module scope for convenience
export using silicon::json::json;

// Backward-compatible alias
export namespace silicon::json {
using JsonValue = json;
}

// Convenience helpers
export namespace silicon::json {
inline json parse(std::string_view text) {
    return json::parse(text, nullptr, false);
}
inline std::string serialize(const json &value) {
    return value.dump();
}
} // namespace silicon::json
