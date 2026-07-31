module;
#include <memory>

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "silicon/core/common.h"
#include "silicon/core/string.hpp"
#include "silicon/core/util.hpp"

export module silicon.exception;

namespace silicon::exception {
export class CORE_API Exception: public std::exception {

  public:
    explicit Exception(std::string_view);

    template<typename... SV>
    Exception(const SV &...args): m_p(std::make_unique<P>()) { m_p->m_message = silicon::util::StrCat(args...); }

    const char *what() const noexcept;

  private:
    struct P {
      public:
      std::string m_message;
    };
    std::unique_ptr<P> m_p;

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
    std::string message;
};

} // namespace silicon::exception

// 泛型结果类型（原 silicon.common 并入 core 后保留 silicon::common 命名空间）。
// 错误类型 E 由调用方按领域指定（如 LLMError），保留语义区分。
namespace silicon::common {

export template<typename T, typename E>
class Result {
    std::variant<T, E> v_;

  public:
    Result(T val): v_(std::move(val)) {}
    Result(E err): v_(std::move(err)) {}
    bool has_value() const { return std::holds_alternative<T>(v_); }
    explicit operator bool() const { return has_value(); }
    T &value() { return std::get<T>(v_); }
    const T &value() const { return std::get<T>(v_); }
    T *operator->() { return &value(); }
    const T *operator->() const { return &value(); }
    E error() const { return std::get<E>(v_); }
};

} // namespace silicon::common

// module silicon.exception;
// module;