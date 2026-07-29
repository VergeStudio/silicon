module;

#include <atomic>
#include <cstdio>
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

inline void AppendPieces(std::string *pDest, std::initializer_list<std::string_view> pieces) {
    size_t size = 0;
    for(const auto &rPiece: pieces) {
        size += rPiece.size();
    }
    pDest->reserve(pDest->size() + size);
    for(const auto &rPiece: pieces) {
        pDest->append(rPiece.data(), rPiece.size());
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
inline void StrAppend(std::string *pDestination, const Args &...args) {
    strings_internal::AppendPieces(pDestination, {static_cast<std::string_view>(args)...});
}

} // namespace silicon::util
