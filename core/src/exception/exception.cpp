module;
#include <memory>

#include <string>

module silicon.exception;

namespace silicon::exception {
Exception::Exception(std::string_view message): m_p(std::make_unique<P>()) {
    m_p->m_message = static_cast<std::string>(message);
}

const char *Exception::what() const noexcept {
    return m_p->m_message.c_str();
}

LogicError::LogicError(std::string_view message): Exception(message) {
}

RuntimeError::RuntimeError(std::string_view message): Exception(message) {
}
} // namespace silicon::exception

// module silicon.exception;
// module;
