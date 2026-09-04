module;


#include <expected>

export module silicon.scheduler:expected;

export namespace silicon::scheduler {
    template <typename T, typename E>
    using expected = std::expected<T, E>;

    template <typename E>
    using unexpected = std::unexpected<E>;
}
