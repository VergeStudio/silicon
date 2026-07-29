#ifndef SILICON_CORE_STRING_HPP
#define SILICON_CORE_STRING_HPP

#include <string>
#include <string_view>

namespace silicon::util {

// -----------------------------------------------------------------------------
// StrCat() — C++23 unified variadic template
// -----------------------------------------------------------------------------
//
// Merges given string-views into a single string with a single allocation.
// Uses fold expressions for efficient concatenation.

namespace strings_internal {

inline void AppendPieces(std::string *pDest, std::initializer_list<std::string_view> pieces) {
    size_t size = 0;
    for (const auto &rPiece : pieces) {
        size += rPiece.size();
    }
    pDest->reserve(pDest->size() + size);
    for (const auto &rPiece : pieces) {
        pDest->append(rPiece.data(), rPiece.size());
    }
}

inline std::string CatPieces(std::initializer_list<std::string_view> pieces) {
    std::string out;
    AppendPieces(&out, std::move(pieces));
    return out;
}

} // namespace strings_internal

// Unified variadic StrCat — handles 0 to N arguments
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

#endif // SILICON_CORE_STRING_HPP
