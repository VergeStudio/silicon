module;
#include <memory>

#include <string>

module silicon.exception;

namespace silicon::exception {
exception::exception(std::string_view message) {
    impl_->message_ = static_cast<std::string>(message);
}

const char *exception::what() const noexcept {
    return impl_->message_.c_str();
}

logic_error::logic_error(std::string_view message): exception(message) {
}

runtime_error::runtime_error(std::string_view message): exception(message) {
}
} // namespace silicon::exception

// module silicon.exception;
// module;
