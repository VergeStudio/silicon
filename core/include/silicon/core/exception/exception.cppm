module;
#include <memory>

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "silicon/core/common.h"

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

    // 异常对象必须可拷贝（[except.throw]），故 Impl 用 shared_ptr 承载 + 深拷贝。
    exception(const exception &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    exception &operator=(const exception &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    exception(exception &&) noexcept = default;
    exception &operator=(exception &&) noexcept = default;
    ~exception() = default;

    const char *what() const noexcept override;

  private:
    struct Impl {
      public:
        std::string message_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};
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

// fs_error — value-style error type carried by silicon.fs result<T>.
// Default-constructible: result<T> 的成功分支会默认构造 error_ 成员。
export class CORE_API fs_error: public runtime_error {
  public:
    fs_error(): runtime_error(std::string_view{}) {}

    template<typename... SV>
    explicit fs_error(const SV &...args): runtime_error(args...) {}
};

// llm_error — value-style error type carried by silicon.llm result<T>.
// spec（llm/specs/llm.md）：`llm_error: { message }`；测试通过 r.error().message 断言。
export struct llm_error {
    struct Impl {
      public:
        std::string message_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

    llm_error() = default;
    explicit llm_error(std::string msg) { impl_->message_ = std::move(msg); }
    llm_error(const llm_error &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    llm_error &operator=(const llm_error &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    llm_error(llm_error &&) noexcept = default;
    llm_error &operator=(llm_error &&) noexcept = default;
    ~llm_error() = default;

    std::string &message() { return impl_->message_; }
    const std::string &message() const { return impl_->message_; }
};

} // namespace silicon::exception

// 泛型结果类型（原 silicon.common 并入 core 后保留 silicon::common 命名空间）。
// 错误类型 E 由调用方按领域指定（如 llm_error），保留语义区分。
namespace silicon::common {

export template<typename T, typename E>
class result {
    struct Impl {
      public:
        std::variant<T, E> v_;
    };
    std::shared_ptr<Impl> impl_;

  public:
    result(T val): impl_(std::make_shared<Impl>(Impl{std::variant<T, E>(std::in_place_index<0>, std::move(val))})) {}
    result(E err): impl_(std::make_shared<Impl>(Impl{std::variant<T, E>(std::in_place_index<1>, std::move(err))})) {}
    result(const result &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    result &operator=(const result &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    result(result &&) noexcept = default;
    result &operator=(result &&) noexcept = default;
    ~result() = default;

    bool has_value() const { return std::holds_alternative<T>(impl_->v_); }
    explicit operator bool() const { return has_value(); }
    T &value() { return std::get<T>(impl_->v_); }
    const T &value() const { return std::get<T>(impl_->v_); }
    T *operator->() { return &value(); }
    const T *operator->() const { return &value(); }
    E error() const { return std::get<E>(impl_->v_); }
};

} // namespace silicon::common

// module silicon.exception;
// module;