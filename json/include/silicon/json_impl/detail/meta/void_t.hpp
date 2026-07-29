//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/silicon/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://silicon.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <silicon/json_impl/detail/abi_macros.hpp>

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
