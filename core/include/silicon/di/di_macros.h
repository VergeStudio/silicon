#pragma once









#define SILICON_DI_CONSTRUCTOR(...)                                                 \
    using di_constructor_type [[maybe_unused]] =                            \
        ::silicon::di::constructor<__VA_ARGS__>;                                     \
    __VA_ARGS__
