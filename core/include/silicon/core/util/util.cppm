module;

#include <atomic>
#include <cstdio>
#include <cstdlib> // silicon::os::GetEnv（_dupenv_s / std::getenv / std::free）
#include <cstring>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>

export module silicon.util;

export namespace silicon::util {

// -----------------------------------------------------------------------------
// util.hpp declarations
// -----------------------------------------------------------------------------

bool HasSuffix(const char *, const char *);
std::uint64_t GenerateUniqueId();

// -----------------------------------------------------------------------------
// string.hpp — StrCat / StrAppend (C++23 unified variadic template)
// -----------------------------------------------------------------------------

namespace strings_internal {

inline void AppendPieces(std::string *dest, std::initializer_list<std::string_view> pieces) {
    size_t size = 0;
    for(const auto &piece: pieces) {
        size += piece.size();
    }
    dest->reserve(dest->size() + size);
    for(const auto &piece: pieces) {
        dest->append(piece.data(), piece.size());
    }
}

inline std::string CatPieces(std::initializer_list<std::string_view> pieces) {
    std::string out;
    AppendPieces(&out, std::move(pieces));
    return out;
}

} // namespace strings_internal

// Unified variadic StrCat — handles 0 to N arguments via fold expression
template <typename... Args>
    requires (std::convertible_to<Args, std::string_view> && ...)
inline std::string StrCat(const Args &...args) {
    return strings_internal::CatPieces({static_cast<std::string_view>(args)...});
}

// Unified variadic StrAppend — handles 1 to N arguments
template <typename... Args>
    requires (std::convertible_to<Args, std::string_view> && ...) && (sizeof...(Args) >= 1)
inline void StrAppend(std::string *destination, const Args &...args) {
    strings_internal::AppendPieces(destination, {static_cast<std::string_view>(args)...});
}

} // namespace silicon::util

// OS 兼容工具（原 silicon.common 并入 core 后保留 silicon::os 命名空间）。
// Windows 使用安全 CRT（_dupenv_s），POSIX 回落 std::getenv，规避
// -Wdeprecated-declarations。
export namespace silicon::os {

// 读取环境变量。未设置或为空时返回空字符串。
inline std::string GetEnv(const char *name) {
#ifdef _WIN32
    char *buf = nullptr;
    size_t len = 0;
    if(_dupenv_s(&buf, &len, name) == 0 && buf != nullptr) {
        std::string value(buf);
        std::free(buf);
        return value;
    }
    return {};
#else
    const char *v = std::getenv(name);
    return (v && *v) ? std::string(v) : std::string{};
#endif
}

} // namespace silicon::os
