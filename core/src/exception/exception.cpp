module;
#include <memory>

#include <string>

module silicon.exception;

namespace silicon::exception {
Exception::Exception(std::string_view message) {
    impl_->message_ = static_cast<std::string>(message);
}

const char *Exception::what() const noexcept {
    return impl_->message_.c_str();
}

LogicError::LogicError(std::string_view message): Exception(message) {
}

RuntimeError::RuntimeError(std::string_view message): Exception(message) {
}
} // namespace silicon::exception

// module silicon.exception;
// module;
