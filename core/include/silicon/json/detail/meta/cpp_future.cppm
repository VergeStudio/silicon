module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

export module silicon.json:detail.meta.cpp_future;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<typename T>
using uncvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

#ifdef JSON_HAS_CPP_14

export using std::enable_if_t;
export using std::index_sequence;
export using std::index_sequence_for;
export using std::make_index_sequence;

#else

export template<bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

export template<typename T, T... Ints>
struct integer_sequence {
    using value_type = T;
    static constexpr std::size_t size() noexcept {
        return sizeof...(Ints);
    }
};

export template<size_t... Ints>
using index_sequence = integer_sequence<size_t, Ints...>;

namespace utility_internal {

export template<typename Seq, size_t SeqSize, size_t Rem>
struct Extend;

export template<typename T, T... Ints, size_t SeqSize>
struct Extend<integer_sequence<T, Ints...>, SeqSize, 0> {
    using type = integer_sequence<T, Ints..., (Ints + SeqSize)...>;
};

export template<typename T, T... Ints, size_t SeqSize>
struct Extend<integer_sequence<T, Ints...>, SeqSize, 1> {
    using type = integer_sequence<T, Ints..., (Ints + SeqSize)..., 2 * SeqSize>;
};

export template<typename T, size_t N>
struct Gen {
    using type =
            typename Extend<typename Gen<T, N / 2>::type, N / 2, N % 2>::type;
};

export template<typename T>
struct Gen<T, 0> {
    using type = integer_sequence<T>;
};

}

export template<typename T, T N>
using make_integer_sequence = typename utility_internal::Gen<T, N>::type;

export template<size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

export template<typename... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

#endif

export template<unsigned N>
struct priority_tag: priority_tag<N - 1> {};
export template<>
struct priority_tag<0> {};

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

}
SILICON_JSON_NAMESPACE_END
