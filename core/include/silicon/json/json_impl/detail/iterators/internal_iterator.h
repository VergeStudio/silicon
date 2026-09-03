//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <silicon/json/json_impl/detail/abi_macros.h>
#include <silicon/json/json_impl/detail/iterators/primitive_iterator.h>

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

/*!
@brief an iterator value

@note This structure could easily be a union, but MSVC currently does not allow
unions members with complex constructors, see https://github.com/silicon/json/pull/105.
*/
template<typename BasicJsonType>
struct internal_iterator {
    /// iterator for JSON objects
    typename BasicJsonType::object_t::iterator object_iterator{};
    /// iterator for JSON arrays
    typename BasicJsonType::array_t::iterator array_iterator{};
    /// generic iterator for all other types
    primitive_iterator_t primitive_iterator{};
};

} // namespace detail
SILICON_JSON_NAMESPACE_END
