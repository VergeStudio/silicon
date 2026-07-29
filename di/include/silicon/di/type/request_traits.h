#pragma once

#include "silicon/di/registration/annotated.h"
#include "silicon/di/type/normalized_type.h"

#include <type_traits>

namespace silicon::di {

template <typename Request>
using request_interface_t = typename annotated_traits<Request>::type;

template <typename Request>
using request_value_t = normalized_type_t<Request>;

template <typename Request>
using request_result_t =
    request_interface_t<std::conditional_t<std::is_rvalue_reference_v<Request>,
                                           std::remove_reference_t<Request>,
                                           Request>>;

template <typename Request, bool RemoveRvalueReferences>
using resolve_request_t =
    std::conditional_t<RemoveRvalueReferences,
                       std::conditional_t<std::is_rvalue_reference_v<Request>,
                                          std::remove_reference_t<Request>,
                                          Request>,
                       Request>;

template <typename Request, bool RemoveRvalueReferences>
using resolve_result_t =
    request_interface_t<resolve_request_t<Request, RemoveRvalueReferences>>;

} // namespace silicon::di
