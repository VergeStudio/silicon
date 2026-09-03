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
#include <array>   // array
#include <cstddef> // size_t
#include <cstdint> // uint8_t
#include <string> // string
#if JSON_HAS_THREE_WAY_COMPARISON
#    include <compare> // partial_ordering
#endif

export module silicon.json:detail.value_t;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

///////////////////////////
// JSON type enumeration //
///////////////////////////

/*!
@brief the JSON type enumeration

This enumeration collects the different JSON types. It is internally used to
distinguish the stored values, and the functions @ref basic_json::is_null(),
@ref basic_json::is_object(), @ref basic_json::is_array(),
@ref basic_json::is_string(), @ref basic_json::is_boolean(),
@ref basic_json::is_number() (with @ref basic_json::is_number_integer(),
@ref basic_json::is_number_unsigned(), and @ref basic_json::is_number_float()),
@ref basic_json::is_discarded(), @ref basic_json::is_primitive(), and
@ref basic_json::is_structured() rely on it.

@note There are three enumeration entries (number_integer, number_unsigned, and
number_float), because the library distinguishes these three types for numbers:
@ref basic_json::number_unsigned_t is used for unsigned integers,
@ref basic_json::number_integer_t is used for signed integers, and
@ref basic_json::number_float_t is used for floating-point numbers or to
approximate integers which do not fit in the limits of their respective type.

@sa see @ref basic_json::basic_json(const value_t value_type) -- create a JSON
value with the default value for a given type

@since version 1.0.0
*/
export enum class value_t : std::uint8_t {
    null,            ///< null value
    object,          ///< object (unordered set of name/value pairs)
    array,           ///< array (ordered collection of values)
    string,          ///< string value
    boolean,         ///< boolean value
    number_integer,  ///< number value (signed integer)
    number_unsigned, ///< number value (unsigned integer)
    number_float,    ///< number value (floating-point)
    binary,          ///< binary array (ordered collection of bytes)
    discarded        ///< discarded by the parser callback function
};

/*!
@brief comparison operator for JSON types

Returns an ordering that is similar to Python:
- order: null < boolean < number < object < array < string < binary
- furthermore, each type is not smaller than itself
- discarded values are not comparable
- binary is represented as a b"" string in python and directly comparable to a
  string; however, making a binary array directly comparable with a string would
  be surprising behavior in a JSON file.

@since version 1.0.0
*/
#if JSON_HAS_THREE_WAY_COMPARISON
export inline std::partial_ordering operator<=>(const value_t lhs, const value_t rhs) noexcept // *NOPAD*
#else
inline bool operator<(const value_t lhs, const value_t rhs) noexcept
#endif
{
    static constexpr std::array<std::uint8_t, 9> order = {{
            0 /* null */, 3 /* object */, 4 /* array */, 5 /* string */,
            1 /* boolean */, 2 /* integer */, 2 /* unsigned */, 2 /* float */,
            6 /* binary */
    }};

    const auto l_index = static_cast<std::size_t>(lhs);
    const auto r_index = static_cast<std::size_t>(rhs);
#if JSON_HAS_THREE_WAY_COMPARISON
    if(l_index < order.size() && r_index < order.size()) {
        return order[l_index] <=> order[r_index]; // *NOPAD*
    }
    return std::partial_ordering::unordered;
#else
    return l_index < order.size() && r_index < order.size() && order[l_index] < order[r_index];
#endif
}

// GCC selects the built-in operator< over an operator rewritten from
// a user-defined spaceship operator
// Clang, MSVC, and ICC select the rewritten candidate
// (see GCC bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105200)
#if JSON_HAS_THREE_WAY_COMPARISON && defined(__GNUC__)
export inline bool operator<(const value_t lhs, const value_t rhs) noexcept {
    return std::is_lt(lhs <=> rhs); // *NOPAD*
}
#endif

} // namespace detail
SILICON_JSON_NAMESPACE_END
