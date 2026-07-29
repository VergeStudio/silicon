module silicon.event;

namespace silicon::event {

Event::Event(std::string name) noexcept: m_name(std::move(name)) {}

auto Event::name() const noexcept -> const std::string & {
    return m_name;
}

auto Event::status() const noexcept -> EventStatus {
    return m_status;
}

void Event::set_status(EventStatus s) noexcept {
    m_status = s;
}

} // namespace silicon::event
