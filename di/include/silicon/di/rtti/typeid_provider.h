#pragma once

#include "silicon/di/core/config.h"
#include "silicon/di/rtti/rtti.h"

#include <typeindex>

namespace silicon::di {

template<> class rtti<typeid_provider> {
    template <typename T> struct wrapper {};

  public:
    class type_index {
        friend struct std::hash<type_index>;
      public:
        type_index(std::type_index value) : value_(value) {}

        bool operator<(const type_index& other) const {
            return value_ < other.value_;
        }
        bool operator==(const type_index& other) const {
            return value_ == other.value_;
        }

      private:       
        std::type_index value_;
    };

    template <typename T> static type_index get_type_index() {
        return std::type_index(typeid(wrapper<T>));
    }
};

} // namespace silicon::di

namespace std {
    template<> struct hash<typename silicon::di::rtti<silicon::di::typeid_provider>::type_index> {
        size_t operator()(const typename silicon::di::rtti<silicon::di::typeid_provider>::type_index& value) const {
            return hash<std::type_index>()(value.value_);
        }
    };
}
