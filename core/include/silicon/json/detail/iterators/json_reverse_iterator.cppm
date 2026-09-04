module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <cstddef>
#include <iterator>
#include <utility>

export module silicon.json:detail.iterators.json_reverse_iterator;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<typename Base>
class json_reverse_iterator: public std::reverse_iterator<Base> {
  public:
    using difference_type = std::ptrdiff_t;

    using base_iterator = std::reverse_iterator<Base>;

    using reference = typename Base::reference;

    explicit json_reverse_iterator(const typename base_iterator::iterator_type &it) noexcept
        : base_iterator(it) {}

    explicit json_reverse_iterator(const base_iterator &it) noexcept: base_iterator(it) {}

    json_reverse_iterator operator++(int) &
    {
        return static_cast<json_reverse_iterator>(base_iterator::operator++(1));
    }

    json_reverse_iterator &operator++() {
        return static_cast<json_reverse_iterator &>(base_iterator::operator++());
    }

    json_reverse_iterator operator--(int) &
    {
        return static_cast<json_reverse_iterator>(base_iterator::operator--(1));
    }

    json_reverse_iterator &operator--() {
        return static_cast<json_reverse_iterator &>(base_iterator::operator--());
    }

    json_reverse_iterator &operator+=(difference_type i) {
        return static_cast<json_reverse_iterator &>(base_iterator::operator+=(i));
    }

    json_reverse_iterator operator+(difference_type i) const {
        return static_cast<json_reverse_iterator>(base_iterator::operator+(i));
    }

    json_reverse_iterator operator-(difference_type i) const {
        return static_cast<json_reverse_iterator>(base_iterator::operator-(i));
    }

    difference_type operator-(const json_reverse_iterator &other) const {
        return base_iterator(*this) - base_iterator(other);
    }

    reference operator[](difference_type n) const {
        return *(this->operator+(n));
    }

    auto key() const -> decltype(std::declval<Base>().key()) {
        auto it = --this->base();
        return it.key();
    }

    reference value() const {
        auto it = --this->base();
        return it.operator*();
    }
};

}
SILICON_JSON_NAMESPACE_END
