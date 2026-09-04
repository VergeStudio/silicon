module;

#include <atomic>
#include <cstdint>
#include <memory>

module silicon.coroutine;

namespace silicon::coroutine {

struct latch::impl {
  public:
    explicit impl(std::int64_t count) noexcept: m_count(count), m_event(count <= 0) {}

    std::atomic<std::int64_t> m_count;

    event m_event;
};

latch::latch(std::int64_t count) noexcept: m_p(std::make_unique<impl>(count)) {}

latch::~latch() = default;

bool latch::is_ready() const noexcept { return m_p->m_event.is_set(); }

std::size_t latch::remaining() const noexcept {
    return static_cast<std::size_t>(m_p->m_count.load(std::memory_order::acquire));
}

void latch::count_down(std::int64_t n) noexcept {
    if(decrement(n)) { m_p->m_event.set(); }
}

bool latch::decrement(std::int64_t n) noexcept {
    return m_p->m_count.fetch_sub(n, std::memory_order::acq_rel) <= n;
}

auto latch::internal_event() noexcept -> event & { return m_p->m_event; }

auto latch::operator co_await() const noexcept -> event::awaiter {
    return m_p->m_event.operator co_await();
}

}
