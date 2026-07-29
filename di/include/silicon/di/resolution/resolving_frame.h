#pragma once

#include "silicon/di/resolution/resolving_frame_fwd.h"
#include "silicon/di/type/type_descriptor.h"

namespace silicon::di {
namespace detail {

class context_path_state;

class resolving_frame {
  public:
    resolving_frame(context_path_state& context, type_descriptor type);

    resolving_frame(const resolving_frame&) = delete;
    resolving_frame& operator=(const resolving_frame&) = delete;
    resolving_frame(resolving_frame&&) = delete;
    resolving_frame& operator=(resolving_frame&&) = delete;

    ~resolving_frame();

  private:
    friend class context_path_state;

    context_path_state* context_;
    resolving_frame* parent_ = nullptr;
    type_descriptor type_;
};

} // namespace detail

} // namespace silicon::di
