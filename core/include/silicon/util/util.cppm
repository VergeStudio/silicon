module;

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <silicon/common.h>

export module silicon.util;

export namespace silicon::util {

SILICON_CORE_API bool has_suffix(const char *, const char *);
SILICON_CORE_API std::uint64_t generate_unique_id();

namespace strings_internal {

inline void append_pieces(std::string *dest, std::initializer_list<std::string_view> pieces) {
    size_t size = 0;
    for(const auto &piece: pieces) {
        size += piece.size();
    }
    dest->reserve(dest->size() + size);
    for(const auto &piece: pieces) {
        dest->append(piece.data(), piece.size());
    }
}

inline std::string cat_pieces(std::initializer_list<std::string_view> pieces) {
    std::string out;
    append_pieces(&out, std::move(pieces));
    return out;
}

}

template <typename... Args>
    requires (std::convertible_to<Args, std::string_view> && ...)
inline std::string str_cat(const Args &...args) {
    return strings_internal::cat_pieces({static_cast<std::string_view>(args)...});
}

template <typename... Args>
    requires (std::convertible_to<Args, std::string_view> && ...) && (sizeof...(Args) >= 1)
inline void str_append(std::string *destination, const Args &...args) {
    strings_internal::append_pieces(destination, {static_cast<std::string_view>(args)...});
}

}

export namespace silicon::os {

inline std::string get_env(const char *name) {
#if defined(SILICON_PLATFORM_WINDOWS)
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

inline void set_env(const char *name, const char *value) {
#if defined(SILICON_PLATFORM_WINDOWS)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

}
