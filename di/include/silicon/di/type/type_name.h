#pragma once

#include "silicon/di/type/type_descriptor.h"

#include <string>

namespace silicon::di {

template <typename T> std::string type_name() {
    std::string name;
    append_type_name(name, describe_type<T>());
    return name;
}

} // namespace silicon::di
