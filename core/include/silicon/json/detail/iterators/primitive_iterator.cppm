












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <silicon/common.h>
#include <cstddef>
#include <limits>

export module silicon.json:detail.iterators.primitive_iterator;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {


export class primitive_iterator_t {
  private:
    using difference_type = std::ptrdiff_t;
    static constexpr difference_type begin_value = 0;
    static constexpr difference_type end_value = begin_value + 1;

    JSON_PRIVATE_UNLESS_TESTED:

        difference_type m_it = (std::numeric_limits<std::ptrdiff_t>::min)();

  public:
    constexpr difference_type get_value() const noexcept {
        return m_it;
    }


    CORE_API void set_begin() noexcept {
        m_it = begin_value;
    }


    CORE_API void set_end() noexcept {
        m_it = end_value;
    }


    constexpr bool is_begin() const noexcept {
        return m_it == begin_value;
    }


    constexpr bool is_end() const noexcept {
        return m_it == end_value;
    }

    friend constexpr bool operator==(primitive_iterator_t lhs, primitive_iterator_t rhs) noexcept {
        return lhs.m_it == rhs.m_it;
    }

    friend constexpr bool operator<(primitive_iterator_t lhs, primitive_iterator_t rhs) noexcept {
        return lhs.m_it < rhs.m_it;
    }

    primitive_iterator_t operator+(difference_type n) noexcept {
        auto result = *this;
        result += n;
        return result;
    }

    friend constexpr difference_type operator-(primitive_iterator_t lhs, primitive_iterator_t rhs) noexcept {
        return lhs.m_it - rhs.m_it;
    }

    CORE_API primitive_iterator_t &operator++() noexcept {
        ++m_it;
        return *this;
    }

    primitive_iterator_t operator++(int) & noexcept
    {
        auto result = *this;
        ++m_it;
        return result;
    }

    primitive_iterator_t &operator--() noexcept {
        --m_it;
        return *this;
    }

    primitive_iterator_t operator--(int) & noexcept
    {
        auto result = *this;
        --m_it;
        return result;
    }

    primitive_iterator_t &operator+=(difference_type n) noexcept {
        m_it += n;
        return *this;
    }

    primitive_iterator_t &operator-=(difference_type n) noexcept {
        m_it -= n;
        return *this;
    }
};

}
SILICON_JSON_NAMESPACE_END
