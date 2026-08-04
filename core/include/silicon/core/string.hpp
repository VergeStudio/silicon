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

inline void AppendPieces(std::string *dest, std::initializer_list<std::string_view> pieces) {
    size_t size = 0;
    for (const auto &piece : pieces) {
        size += piece.size();
    }
    dest->reserve(dest->size() + size);
    for (const auto &piece : pieces) {
        dest->append(piece.data(), piece.size());
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
inline void StrAppend(std::string *destination, const Args &...args) {
    strings_internal::AppendPieces(destination, {static_cast<std::string_view>(args)...});
}

} // namespace silicon::util

#endif // SILICON_CORE_STRING_HPP
