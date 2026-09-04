module;


#include <string>
#include <utility>

module silicon.event;

namespace silicon::event {

event::event(std::string name) noexcept {
    impl_->name_ = std::move(name);
}

auto event::name() const noexcept -> const std::string & {
    return impl_->name_;
}

auto event::status() const noexcept -> event_status {
    return impl_->status_;
}

void event::set_status(event_status s) noexcept {
    impl_->status_ = s;
}

}
