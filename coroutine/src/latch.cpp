module;

#include <atomic>
#include <cstdint>
#include <memory>

module silicon.coroutine;

namespace silicon::coroutine {

struct latch::Impl {
  public:
    explicit Impl(std::int64_t count) noexcept: m_count(count), m_event(count <= 0) {}

    /// The number of tasks to wait for completion before triggering the event to resume.
    std::atomic<std::int64_t> m_count;
    /// The event to trigger when the latch counter reaches zero, this resumes the coroutine
    /// that is co_await'ing on the latch.
    event m_event;
};

latch::latch(std::int64_t count) noexcept: m_p(std::make_unique<Impl>(count)) {}

latch::~latch() = default;

auto latch::is_ready() const noexcept -> bool { return m_p->m_event.is_set(); }

auto latch::remaining() const noexcept -> std::size_t {
    return static_cast<std::size_t>(m_p->m_count.load(std::memory_order::acquire));
}

auto latch::count_down(std::int64_t n) noexcept -> void {
    if(decrement(n)) { m_p->m_event.set(); }
}

auto latch::decrement(std::int64_t n) noexcept -> bool {
    return m_p->m_count.fetch_sub(n, std::memory_order::acq_rel) <= n;
}

auto latch::internal_event() noexcept -> event & { return m_p->m_event; }

auto latch::operator co_await() const noexcept -> event::awaiter {
    return m_p->m_event.operator co_await();
}

} // namespace silicon::coroutine
