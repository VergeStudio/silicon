//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <silicon/json/json_impl/detail/abi_macros.h>

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

template<typename... Ts>
struct make_void {
    using type = void;
};
template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

} // namespace detail
SILICON_JSON_NAMESPACE_END
