//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <silicon/json/impl/detail/abi_macros.h>

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

// dispatching helper struct
template<class T>
struct identity_tag {};

} // namespace detail
SILICON_JSON_NAMESPACE_END
