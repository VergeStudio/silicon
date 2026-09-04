module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <iterator>
#include <type_traits>

export module silicon.json:detail.iterators.iter_impl;

import :detail.exceptions;
import :detail.iterators.internal_iterator;
import :detail.iterators.primitive_iterator;
import :detail.meta.cpp_future;
import :detail.meta.type_traits;
import :detail.value_t;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<typename IteratorType>
class iteration_proxy;
export template<typename IteratorType>
class iteration_proxy_value;

export template<typename BasicJsonType>
class iter_impl
{

    using other_iter_impl = iter_impl<typename std::conditional<std::is_const<BasicJsonType>::value, typename std::remove_const<BasicJsonType>::type, const BasicJsonType>::type>;

    friend other_iter_impl;
    friend BasicJsonType;
    friend iteration_proxy<iter_impl>;
    friend iteration_proxy_value<iter_impl>;

    using object_t = typename BasicJsonType::object_t;
    using array_t = typename BasicJsonType::array_t;

    static_assert(is_basic_json<typename std::remove_const<BasicJsonType>::type>::value, "iter_impl only accepts (const) basic_json");

    static_assert(std::is_base_of<std::bidirectional_iterator_tag, std::bidirectional_iterator_tag>::value && std::is_base_of<std::bidirectional_iterator_tag, typename std::iterator_traits<typename array_t::iterator>::iterator_category>::value, "basic_json iterator assumes array and object type iterators satisfy the LegacyBidirectionalIterator named requirement.");

  public:

    using iterator_category = std::bidirectional_iterator_tag;

    using value_type = typename BasicJsonType::value_type;

    using difference_type = typename BasicJsonType::difference_type;

    using pointer = typename std::conditional<std::is_const<BasicJsonType>::value, typename BasicJsonType::const_pointer, typename BasicJsonType::pointer>::type;

    using reference =
            typename std::conditional<std::is_const<BasicJsonType>::value, typename BasicJsonType::const_reference, typename BasicJsonType::reference>::type;

    iter_impl() = default;
    ~iter_impl() = default;
    iter_impl(iter_impl &&) noexcept = default;
    iter_impl &operator=(iter_impl &&) noexcept = default;

    explicit iter_impl(pointer object) noexcept: m_object(object) {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object: {
                m_it.object_iterator = typename object_t::iterator();
                break;
            }

            case value_t::array: {
                m_it.array_iterator = typename array_t::iterator();
                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                m_it.primitive_iterator = primitive_iterator_t();
                break;
            }
        }
    }

    iter_impl(const iter_impl<const BasicJsonType> &other) noexcept
        : m_object(other.m_object), m_it(other.m_it) {}

    iter_impl &operator=(const iter_impl<const BasicJsonType> &other) noexcept {
        if(&other != this) {
            m_object = other.m_object;
            m_it = other.m_it;
        }
        return *this;
    }

    iter_impl(const iter_impl<typename std::remove_const<BasicJsonType>::type> &other) noexcept
        : m_object(other.m_object), m_it(other.m_it) {}

    iter_impl &operator=(const iter_impl<typename std::remove_const<BasicJsonType>::type> &other) noexcept
    {
        m_object = other.m_object;
        m_it = other.m_it;
        return *this;
    }

    JSON_PRIVATE_UNLESS_TESTED:

        void
        set_begin() noexcept {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object: {
                m_it.object_iterator = m_object->m_data.m_value.object->begin();
                break;
            }

            case value_t::array: {
                m_it.array_iterator = m_object->m_data.m_value.array->begin();
                break;
            }

            case value_t::null: {

                m_it.primitive_iterator.set_end();
                break;
            }

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                m_it.primitive_iterator.set_begin();
                break;
            }
        }
    }

    void set_end() noexcept {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object: {
                m_it.object_iterator = m_object->m_data.m_value.object->end();
                break;
            }

            case value_t::array: {
                m_it.array_iterator = m_object->m_data.m_value.array->end();
                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                m_it.primitive_iterator.set_end();
                break;
            }
        }
    }

  public:

    reference operator*() const {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object: {
                JSON_ASSERT(m_it.object_iterator != m_object->m_data.m_value.object->end());
                return m_it.object_iterator->second;
            }

            case value_t::array: {
                JSON_ASSERT(m_it.array_iterator != m_object->m_data.m_value.array->end());
                return *m_it.array_iterator;
            }

            case value_t::null:
                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                if(JSON_HEDLEY_LIKELY(m_it.primitive_iterator.is_begin())) {
                    return *m_object;
                }

                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));
            }
        }
    }

    pointer operator->() const {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object: {
                JSON_ASSERT(m_it.object_iterator != m_object->m_data.m_value.object->end());
                return &(m_it.object_iterator->second);
            }

            case value_t::array: {
                JSON_ASSERT(m_it.array_iterator != m_object->m_data.m_value.array->end());
                return &*m_it.array_iterator;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                if(JSON_HEDLEY_LIKELY(m_it.primitive_iterator.is_begin())) {
                    return m_object;
                }

                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));
            }
        }
    }

    iter_impl operator++(int) &
    {
        auto result = *this;
        ++(*this);
        return result;
    }

    iter_impl &operator++() {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object: {
                std::advance(m_it.object_iterator, 1);
                break;
            }

            case value_t::array: {
                std::advance(m_it.array_iterator, 1);
                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                ++m_it.primitive_iterator;
                break;
            }
        }

        return *this;
    }

    iter_impl operator--(int) &
    {
        auto result = *this;
        --(*this);
        return result;
    }

    iter_impl &operator--() {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object: {
                std::advance(m_it.object_iterator, -1);
                break;
            }

            case value_t::array: {
                std::advance(m_it.array_iterator, -1);
                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                --m_it.primitive_iterator;
                break;
            }
        }

        return *this;
    }

    template<typename IterImpl, detail::enable_if_t<(std::is_same<IterImpl, iter_impl>::value || std::is_same<IterImpl, other_iter_impl>::value), std::nullptr_t> = nullptr>
    bool operator==(const IterImpl &other) const {

        if(JSON_HEDLEY_UNLIKELY(m_object != other.m_object)) {
            JSON_THROW(invalid_iterator::create(212, "cannot compare iterators of different containers", m_object));
        }

        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object:
                return (m_it.object_iterator == other.m_it.object_iterator);

            case value_t::array:
                return (m_it.array_iterator == other.m_it.array_iterator);

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                return (m_it.primitive_iterator == other.m_it.primitive_iterator);
        }
    }

    template<typename IterImpl, detail::enable_if_t<(std::is_same<IterImpl, iter_impl>::value || std::is_same<IterImpl, other_iter_impl>::value), std::nullptr_t> = nullptr>
    bool operator!=(const IterImpl &other) const {
        return !operator==(other);
    }

    bool operator<(const iter_impl &other) const {

        if(JSON_HEDLEY_UNLIKELY(m_object != other.m_object)) {
            JSON_THROW(invalid_iterator::create(212, "cannot compare iterators of different containers", m_object));
        }

        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object:
                JSON_THROW(invalid_iterator::create(213, "cannot compare order of object iterators", m_object));

            case value_t::array:
                return (m_it.array_iterator < other.m_it.array_iterator);

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                return (m_it.primitive_iterator < other.m_it.primitive_iterator);
        }
    }

    bool operator<=(const iter_impl &other) const {
        return !other.operator<(*this);
    }

    bool operator>(const iter_impl &other) const {
        return !operator<=(other);
    }

    bool operator>=(const iter_impl &other) const {
        return !operator<(other);
    }

    iter_impl &operator+=(difference_type i) {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object:
                JSON_THROW(invalid_iterator::create(209, "cannot use offsets with object iterators", m_object));

            case value_t::array: {
                std::advance(m_it.array_iterator, i);
                break;
            }

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                m_it.primitive_iterator += i;
                break;
            }
        }

        return *this;
    }

    iter_impl &operator-=(difference_type i) {
        return operator+=(-i);
    }

    iter_impl operator+(difference_type i) const {
        auto result = *this;
        result += i;
        return result;
    }

    friend iter_impl operator+(difference_type i, const iter_impl &it) {
        auto result = it;
        result += i;
        return result;
    }

    iter_impl operator-(difference_type i) const {
        auto result = *this;
        result -= i;
        return result;
    }

    difference_type operator-(const iter_impl &other) const {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object:
                JSON_THROW(invalid_iterator::create(209, "cannot use offsets with object iterators", m_object));

            case value_t::array:
                return m_it.array_iterator - other.m_it.array_iterator;

            case value_t::null:
            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default:
                return m_it.primitive_iterator - other.m_it.primitive_iterator;
        }
    }

    reference operator[](difference_type n) const {
        JSON_ASSERT(m_object != nullptr);

        switch(m_object->m_data.m_type) {
            case value_t::object:
                JSON_THROW(invalid_iterator::create(208, "cannot use operator[] for object iterators", m_object));

            case value_t::array:
                return *std::next(m_it.array_iterator, n);

            case value_t::null:
                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));

            case value_t::string:
            case value_t::boolean:
            case value_t::number_integer:
            case value_t::number_unsigned:
            case value_t::number_float:
            case value_t::binary:
            case value_t::discarded:
            default: {
                if(JSON_HEDLEY_LIKELY(m_it.primitive_iterator.get_value() == -n)) {
                    return *m_object;
                }

                JSON_THROW(invalid_iterator::create(214, "cannot get value", m_object));
            }
        }
    }

    const typename object_t::key_type &key() const {
        JSON_ASSERT(m_object != nullptr);

        if(JSON_HEDLEY_LIKELY(m_object->is_object())) {
            return m_it.object_iterator->first;
        }

        JSON_THROW(invalid_iterator::create(207, "cannot use key() for non-object iterators", m_object));
    }

    reference value() const {
        return operator*();
    }

    JSON_PRIVATE_UNLESS_TESTED:

        pointer m_object = nullptr;

    internal_iterator<typename std::remove_const<BasicJsonType>::type> m_it{};
};

}
SILICON_JSON_NAMESPACE_END
