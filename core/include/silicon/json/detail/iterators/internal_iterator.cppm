//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

// Partition of the silicon.json module. Macros (JSON_* feature
// flags, SILICON_JSON_NAMESPACE_* ) are NOT exported by C++20
// modules, so the macro headers are textually included in the
// global module fragment of every partition that needs them.

module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>

export module silicon.json:detail.iterators.internal_iterator;

import :detail.iterators.primitive_iterator;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

/*!
@brief an iterator value

@note This structure could easily be a union, but MSVC currently does not allow
unions members with complex constructors, see https://github.com/silicon/json/pull/105.
*/
export template<typename BasicJsonType>
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
