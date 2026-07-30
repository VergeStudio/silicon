module;


#include <expected>

export module silicon.coroutine:expected;

export namespace silicon::coroutine {
    template <typename T, typename E>
    using expected = std::expected<T, E>;

    template <typename E>
    using unexpected = std::unexpected<E>;
} // namespace silicon::coroutine
