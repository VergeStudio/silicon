module;


#include <concepts>
#include <ranges>

export module silicon.scheduler:concepts.range_of;

export namespace silicon::scheduler::concepts {

template<class T, class V>
concept range_of = std::ranges::range<T> && std::is_same_v<V, std::ranges::range_value_t<T>>;


template<class T, class V>
concept sized_range_of = std::ranges::sized_range<T> && std::is_same_v<V, std::ranges::range_value_t<T>>;

}
