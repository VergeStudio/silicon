#pragma once

#include "silicon/di/core/config.h"

#include <type_traits>
#include <utility>

namespace silicon::di {
template <typename TargetType, typename SourceType, typename = void>
struct type_conversion_traits {
    template <typename Source>
    static TargetType convert(Source&& source) {
        static_assert(
            std::is_constructible_v<TargetType, Source&&>,
            "type conversion requires a type_conversion_traits "
            "specialization or a direct converting constructor");
        return TargetType(std::forward<Source>(source));
    }
};

template <typename Target, typename Source>
struct type_conversion_traits<Target*, Source*> {
    static Target* convert(Source* source) { return static_cast<Target*>(source); }
};
} // namespace silicon::di
