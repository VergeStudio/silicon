#pragma once

#include "silicon/di/core/config.h"

#include "silicon/di/rtti/rtti.h"

namespace silicon::di {

template<> class rtti<static_provider> {
    template <typename T> struct type_index_tag {
        // TODO: This will not work across modules.
        // Should probably detect it first, handle it next.
        static constexpr size_t tag{};
    };

  public:
    class type_index {
        friend struct std::hash<type_index>;
      public:
        constexpr type_index(size_t value) : value_(value) {}

        constexpr bool operator<(const type_index& other) const {
            return value_ < other.value_;
        }

        constexpr bool operator==(const type_index& other) const {
            return value_ == other.value_;
        }

      private:
        size_t value_;
    };

    template <typename T> static constexpr type_index get_type_index() {
        return reinterpret_cast<size_t>(&type_index_tag<T>::tag);
    }
};
} // namespace silicon::di

namespace std {
    template<> struct hash<typename silicon::di::rtti<silicon::di::static_provider>::type_index> {
        size_t operator()(const typename silicon::di::rtti<silicon::di::static_provider>::type_index& value) const {
            return hash<size_t>()(value.value_);
        }
    };
}
