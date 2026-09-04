












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

export module silicon.json:ordered_map;

import :detail.meta.type_traits;


SILICON_JSON_NAMESPACE_BEGIN



export template<class Key, class T, class IgnoredLess = std::less<Key>, class Allocator = std::allocator<std::pair<const Key, T>>>
struct ordered_map: std::vector<std::pair<const Key, T>, Allocator> {
    using key_type = Key;
    using mapped_type = T;
    using Container = std::vector<std::pair<const Key, T>, Allocator>;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using size_type = typename Container::size_type;
    using value_type = typename Container::value_type;
#ifdef JSON_HAS_CPP_14
    using key_compare = std::equal_to<>;
#else
    using key_compare = std::equal_to<Key>;
#endif



    ordered_map() noexcept(noexcept(Container())): Container{} {}
    explicit ordered_map(const Allocator &alloc) noexcept(noexcept(Container(alloc))): Container{alloc} {}
    template<class It>
    ordered_map(It first, It last, const Allocator &alloc = Allocator())
        : Container{first, last, alloc} {}
    ordered_map(std::initializer_list<value_type> init, const Allocator &alloc = Allocator())
        : Container{init, alloc} {}

    std::pair<iterator, bool> emplace(const key_type &key, T &&t) {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return {it, false};
            }
        }
        Container::emplace_back(key, std::forward<T>(t));
        return {std::prev(this->end()), true};
    }

    template<class KeyType, detail::enable_if_t<detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
    std::pair<iterator, bool> emplace(KeyType &&key, T &&t) {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return {it, false};
            }
        }
        Container::emplace_back(std::forward<KeyType>(key), std::forward<T>(t));
        return {std::prev(this->end()), true};
    }

    T &operator[](const key_type &key) {
        return emplace(key, T{}).first->second;
    }

    template<class KeyType, detail::enable_if_t<detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
    T &operator[](KeyType &&key) {
        return emplace(std::forward<KeyType>(key), T{}).first->second;
    }

    const T &operator[](const key_type &key) const {
        return at(key);
    }

    template<class KeyType, detail::enable_if_t<detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
    const T &operator[](KeyType &&key) const {
        return at(std::forward<KeyType>(key));
    }

    T &at(const key_type &key) {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return it->second;
            }
        }

        JSON_THROW(std::out_of_range("key not found"));
    }

    template<class KeyType, detail::enable_if_t<detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
    T &at(KeyType &&key)
    {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return it->second;
            }
        }

        JSON_THROW(std::out_of_range("key not found"));
    }

    const T &at(const key_type &key) const {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return it->second;
            }
        }

        JSON_THROW(std::out_of_range("key not found"));
    }

    template<class KeyType, detail::enable_if_t<detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
    const T &at(KeyType &&key) const
    {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return it->second;
            }
        }

        JSON_THROW(std::out_of_range("key not found"));
    }

    size_type erase(const key_type &key) {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {

                for(auto next = it; ++next != this->end(); ++it) {
                    it->~value_type();
                    new(&*it) value_type{std::move(*next)};
                }
                Container::pop_back();
                return 1;
            }
        }
        return 0;
    }

    template<class KeyType, detail::enable_if_t<detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
    size_type erase(KeyType &&key)
    {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {

                for(auto next = it; ++next != this->end(); ++it) {
                    it->~value_type();
                    new(&*it) value_type{std::move(*next)};
                }
                Container::pop_back();
                return 1;
            }
        }
        return 0;
    }

    iterator erase(iterator pos) {
        return erase(pos, std::next(pos));
    }

    iterator erase(iterator first, iterator last) {
        if(first == last) {
            return first;
        }

        const auto elements_affected = std::distance(first, last);
        const auto offset = std::distance(Container::begin(), first);





















        for(auto it = first; std::next(it, elements_affected) != Container::end(); ++it) {
            it->~value_type();
            new(&*it) value_type{std::move(*std::next(it, elements_affected))};
        }






        Container::resize(this->size() - static_cast<size_type>(elements_affected));








        return Container::begin() + offset;
    }

    size_type count(const key_type &key) const {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return 1;
            }
        }
        return 0;
    }

    template<class KeyType, detail::enable_if_t<detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
    size_type count(KeyType &&key) const
    {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return 1;
            }
        }
        return 0;
    }

    iterator find(const key_type &key) {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return it;
            }
        }
        return Container::end();
    }

    template<class KeyType, detail::enable_if_t<detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
    iterator find(KeyType &&key)
    {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return it;
            }
        }
        return Container::end();
    }

    const_iterator find(const key_type &key) const {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, key)) {
                return it;
            }
        }
        return Container::end();
    }

    std::pair<iterator, bool> insert(value_type &&value) {
        return emplace(value.first, std::move(value.second));
    }

    std::pair<iterator, bool> insert(const value_type &value) {
        for(auto it = this->begin(); it != this->end(); ++it) {
            if(m_compare(it->first, value.first)) {
                return {it, false};
            }
        }
        Container::push_back(value);
        return {--this->end(), true};
    }

    template<typename InputIt>
    using require_input_iter = typename std::enable_if<std::is_convertible<typename std::iterator_traits<InputIt>::iterator_category, std::input_iterator_tag>::value>::type;

    template<typename InputIt, typename = require_input_iter<InputIt>>
    void insert(InputIt first, InputIt last) {
        for(auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

  private:
    JSON_NO_UNIQUE_ADDRESS key_compare m_compare = key_compare();
};

SILICON_JSON_NAMESPACE_END
