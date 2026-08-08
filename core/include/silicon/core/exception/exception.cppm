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
export class CORE_API Exception: public std::exception {

  public:
    explicit Exception(std::string_view);

    template<typename... SV>
    Exception(const SV &...args) {
        impl_->message_ = silicon::util::StrCat(args...);
    }

    // 异常对象必须可拷贝（[except.throw]），故 Impl 用 shared_ptr 承载 + 深拷贝。
    Exception(const Exception &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    Exception &operator=(const Exception &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    Exception(Exception &&) noexcept = default;
    Exception &operator=(Exception &&) noexcept = default;
    ~Exception() = default;

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
export class CORE_API LogicError: public Exception {
  public:
    explicit LogicError(std::string_view);

    template<typename... SV>
    LogicError(const SV &...args): Exception(args...) {}
};

// This errors are usually related to problems that are relted to data or conditions
// that happen only at run-time
export class CORE_API RuntimeError: public Exception {
  public:
    explicit RuntimeError(std::string_view);

    template<typename... SV>
    RuntimeError(const SV &...args): Exception(args...) {}
};

// Thrown when an operation is intentionally not implemented (e.g. proxy
// weak_dispatch fallback). Default-constructible: call sites throw
// `not_implemented{}` without a message.
export class CORE_API not_implemented: public LogicError {
  public:
    not_implemented(): LogicError("not implemented") {}

    template<typename... SV>
    explicit not_implemented(const SV &...args): LogicError(args...) {}
};

// FsError — value-style error type carried by silicon.fs Result<T>.
// Default-constructible: Result<T> 的成功分支会默认构造 error_ 成员。
export class CORE_API FsError: public RuntimeError {
  public:
    FsError(): RuntimeError(std::string_view{}) {}

    template<typename... SV>
    explicit FsError(const SV &...args): RuntimeError(args...) {}
};

// LLMError — value-style error type carried by silicon.llm Result<T>.
// spec（llm/specs/llm.md）：`LLMError: { message }`；测试通过 r.error().message 断言。
export struct LLMError {
    struct Impl {
      public:
        std::string message_;
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

    LLMError() = default;
    explicit LLMError(std::string msg) { impl_->message_ = std::move(msg); }
    LLMError(const LLMError &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    LLMError &operator=(const LLMError &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    LLMError(LLMError &&) noexcept = default;
    LLMError &operator=(LLMError &&) noexcept = default;
    ~LLMError() = default;

    std::string &message() { return impl_->message_; }
    const std::string &message() const { return impl_->message_; }
};

} // namespace silicon::exception

// 泛型结果类型（原 silicon.common 并入 core 后保留 silicon::common 命名空间）。
// 错误类型 E 由调用方按领域指定（如 LLMError），保留语义区分。
namespace silicon::common {

export template<typename T, typename E>
class Result {
    struct Impl {
      public:
        std::variant<T, E> v_;
    };
    std::shared_ptr<Impl> impl_;

  public:
    Result(T val): impl_(std::make_shared<Impl>(Impl{std::variant<T, E>(std::in_place_index<0>, std::move(val))})) {}
    Result(E err): impl_(std::make_shared<Impl>(Impl{std::variant<T, E>(std::in_place_index<1>, std::move(err))})) {}
    Result(const Result &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    Result &operator=(const Result &o) {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    Result(Result &&) noexcept = default;
    Result &operator=(Result &&) noexcept = default;
    ~Result() = default;

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