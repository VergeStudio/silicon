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
#include <type_traits> // conditional, enable_if, false_type, integral_constant, is_constructible, is_integral, is_same, remove_cv, remove_reference, true_type
#include <utility>     // index_sequence, make_index_sequence, index_sequence_for

export module silicon.json:detail.meta.cpp_future;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<typename T>
using uncvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

#ifdef JSON_HAS_CPP_14

// the following utilities are natively available in C++14
export using std::enable_if_t;
export using std::index_sequence;
export using std::index_sequence_for;
export using std::make_index_sequence;

#else

// alias templates to reduce boilerplate
export template<bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

// The following code is taken from https://github.com/abseil/abseil-cpp/blob/10cb35e459f5ecca5b2ff107635da0bfa41011b4/absl/utility/utility.h
// which is part of Google Abseil (https://github.com/abseil/abseil-cpp), licensed under the Apache License 2.0.

//// START OF CODE FROM GOOGLE ABSEIL

// integer_sequence
//
// Class template representing a compile-time integer sequence. An instantiation
// of `integer_sequence<T, Ints...>` has a sequence of integers encoded in its
// type through its template arguments (which is a common need when
// working with C++11 variadic templates). `absl::integer_sequence` is designed
// to be a drop-in replacement for C++14's `std::integer_sequence`.
//
// Example:
//
//   template< class T, T... Ints >
//   void user_function(integer_sequence<T, Ints...>);
//
//   int main()
//   {
//     // user_function's `T` will be deduced to `int` and `Ints...`
//     // will be deduced to `0, 1, 2, 3, 4`.
//     user_function(make_integer_sequence<int, 5>());
//   }
export template<typename T, T... Ints>
struct integer_sequence {
    using value_type = T;
    static constexpr std::size_t size() noexcept {
        return sizeof...(Ints);
    }
};

// index_sequence
//
// A helper template for an `integer_sequence` of `size_t`,
// `absl::index_sequence` is designed to be a drop-in replacement for C++14's
// `std::index_sequence`.
export template<size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

namespace utility_internal {

export template<typename Seq, size_t SeqSize, size_t Rem>
struct Extend;

// Note that SeqSize == sizeof...(Ints). It's passed explicitly for efficiency.
export template<typename T, T... Ints, size_t SeqSize>
struct Extend<integer_sequence<T, Ints...>, SeqSize, 0> {
    using type = integer_sequence<T, Ints..., (Ints + SeqSize)...>;
};

export template<typename T, T... Ints, size_t SeqSize>
struct Extend<integer_sequence<T, Ints...>, SeqSize, 1> {
    using type = integer_sequence<T, Ints..., (Ints + SeqSize)..., 2 * SeqSize>;
};

// Recursion helper for 'make_integer_sequence<T, N>'.
// 'Gen<T, N>::type' is an alias for 'integer_sequence<T, 0, 1, ... N-1>'.
export template<typename T, size_t N>
struct Gen {
    using type =
            typename Extend<typename Gen<T, N / 2>::type, N / 2, N % 2>::type;
};

export template<typename T>
struct Gen<T, 0> {
    using type = integer_sequence<T>;
};

} // namespace utility_internal

// Compile-time sequences of integers

// make_integer_sequence
//
// This template alias is equivalent to
// `integer_sequence<int, 0, 1, ..., N-1>`, and is designed to be a drop-in
// replacement for C++14's `std::make_integer_sequence`.
export template<typename T, T N>
using make_integer_sequence = typename utility_internal::Gen<T, N>::type;

// make_index_sequence
//
// This template alias is equivalent to `index_sequence<0, 1, ..., N-1>`,
// and is designed to be a drop-in replacement for C++14's
// `std::make_index_sequence`.
export template<size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

// index_sequence_for
//
// Converts a typename pack into an index sequence of the same length, and
// is designed to be a drop-in replacement for C++14's
// `std::index_sequence_for()`
export template<typename... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

//// END OF CODE FROM GOOGLE ABSEIL

#endif

// dispatch utility (taken from ranges-v3)
export template<unsigned N>
struct priority_tag: priority_tag<N - 1> {};
export template<>
struct priority_tag<0> {};

// taken from ranges-v3
export template<typename T>
struct static_const {
    static JSON_INLINE_VARIABLE constexpr T value{};
};

#ifndef JSON_HAS_CPP_17
export template<typename T>
constexpr T static_const<T>::value;
#endif

export template<typename T, typename... Args>
inline constexpr std::array<T, sizeof...(Args)> make_array(Args &&...args) {
    return std::array<T, sizeof...(Args)>{{static_cast<T>(std::forward<Args>(args))...}};
}

} // namespace detail
SILICON_JSON_NAMESPACE_END
