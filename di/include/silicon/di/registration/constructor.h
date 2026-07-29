#pragma once

// Intentionally without any includes

namespace silicon::di {

template <typename...> struct constructor;
#define DINGO_CONSTRUCTOR(...)                                                 \
    using di_constructor_type [[maybe_unused]] =                            \
        ::silicon::di::constructor<__VA_ARGS__>;                                     \
    __VA_ARGS__

} // namespace silicon::di
