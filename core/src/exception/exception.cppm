module;

#include <stdexcept>
#include <string>

#include "silicon/core/common.h"
#include "silicon/core/string.hpp"
#include "silicon/core/util.hpp"

export module silicon.exception;

namespace silicon::exception {
export class CORE_API Exception: public std::exception {
  public:
    explicit Exception(std::string_view);

    template<typename... SV>
    Exception(const SV &...args): m_message(silicon::util::StrCat(args...)) {}

    const char *what() const noexcept;

  private:
    std::string m_message;
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

} // namespace silicon::exception

// module silicon.exception;
// module;