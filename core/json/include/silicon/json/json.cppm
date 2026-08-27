module;

export module silicon.json;

// Consume the C++20 standard library module so that std comparison operators
// (e.g. unique_ptr != nullptr inside basic_json::create) are visible across
// module boundaries when importers instantiate basic_json.
import std;

// silicon.json_impl 已把完整 JSON 实现内联进自身 global module fragment
// （原 json.hpp 伞头已删除），作为真正的命名模块只产出一份 BMI；消费方只
// import 该模块，故无共享 header-unit 缓存条目，消除 clean -j4 C3474 竞态。
export import silicon.json_impl;

// Public type, re-exported from the forked implementation.
// 注意：clang 下 export import 仅暴露 json_impl 在模块作用域重导出的 json，
// 不暴露 silicon::json_impl 命名空间路径；故此处用模块作用域的 json（即
// json_impl 重导出名）作别名源，MSVC/clang 均可编译。
export namespace silicon::json {
using json = ::json;
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
