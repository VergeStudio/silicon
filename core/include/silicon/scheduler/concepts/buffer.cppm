module;

#include <concepts>
#include <cstdint>
#include <type_traits>

export module silicon.scheduler:concepts.buffer;

export namespace silicon::scheduler::concepts {

template<typename type>
concept const_buffer = requires(const type t)
{

    typename std::remove_pointer_t<decltype(t.data())>;
    requires std::is_trivial_v<std::remove_pointer_t<decltype(t.data())>>;

    { t.empty() } -> std::same_as<bool>;
    { t.size() } -> std::same_as<std::size_t>;

    { t.data() } -> std::convertible_to<const typename std::remove_pointer_t<decltype(t.data())>*>;
};

template<const_buffer buffer_type>
struct const_buffer_traits
{
    using element_type = std::add_const_t<std::remove_pointer_t<decltype(std::declval<buffer_type>().data())>>;
};

template<typename type>
concept mutable_buffer = requires(type t)
{

    typename std::remove_pointer_t<decltype(t.data())>;
    requires std::is_trivial_v<std::remove_pointer_t<decltype(t.data())>>;

    { t.empty() } -> std::same_as<bool>;
    { t.size() } -> std::same_as<std::size_t>;

    { t.data() } -> std::same_as<typename std::remove_pointer_t<decltype(t.data())>*>;
};

template<mutable_buffer buffer_type>
struct mutable_buffer_traits
{
    using element_type = std::remove_pointer_t<decltype(std::declval<buffer_type>().data())>;
};

}
