module;
#include <memory>

#include <stdexcept>
#include <string>
#include <utility>

#include "silicon/common.h"

export module silicon.exception;

import silicon.util;

namespace silicon::exception {
export class CORE_API exception: public std::exception {

  public:
    explicit exception(std::string_view);

    template<typename... SV>
    exception(const SV &...args) {
        impl_->message_ = silicon::util::str_cat(args...);
    }

    // 异常对象必须可拷贝（[except.throw]），故 impl 用 shared_ptr 承载 + 深拷贝。
    exception(const exception &o): impl_(std::make_shared<impl>(*o.impl_)) {}
    exception &operator=(const exception &o) {
        if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
        return *this;
    }
    exception(exception &&) noexcept = default;
    exception &operator=(exception &&) noexcept = default;
    ~exception() = default;

    const char *what() const noexcept override;

  private:
    struct impl {
      public:
        std::string message_;
    };
    std::shared_ptr<impl> impl_{std::make_shared<impl>()};
};

// This errors are usually related to problems which "probably" require code refactoring
// to be fixed.
export class CORE_API logic_error: public exception {
  public:
    explicit logic_error(std::string_view);

    template<typename... SV>
    logic_error(const SV &...args): exception(args...) {}
};

// This errors are usually related to problems that are relted to data or conditions
// that happen only at run-time
export class CORE_API runtime_error: public exception {
  public:
    explicit runtime_error(std::string_view);

    template<typename... SV>
    runtime_error(const SV &...args): exception(args...) {}
};

// Thrown when an operation is intentionally not implemented (e.g. proxy
// weak_dispatch fallback). Default-constructible: call sites throw
// `not_implemented{}` without a message.
export class CORE_API not_implemented: public logic_error {
  public:
    not_implemented(): logic_error("not implemented") {}

    template<typename... SV>
    explicit not_implemented(const SV &...args): logic_error(args...) {}
};

} // namespace silicon::exception

// module silicon.exception;