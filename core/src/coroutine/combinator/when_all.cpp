module;

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <memory>
#include <utility>

module silicon.coroutine;

namespace silicon::coroutine {

struct when_all_latch::impl {
  public:

    std::atomic<std::size_t> m_count;

    std::coroutine_handle<> m_awaiting_coroutine{nullptr};
};

when_all_latch::when_all_latch(std::size_t count) noexcept: m_p(std::make_unique<impl>()) {
    m_p->m_count = count + 1;
}

when_all_latch::when_all_latch(when_all_latch &&other): m_p(std::make_unique<impl>()) {
    m_p->m_count              = other.m_p->m_count.load(std::memory_order::acquire);
    m_p->m_awaiting_coroutine = std::exchange(other.m_p->m_awaiting_coroutine, nullptr);
}

when_all_latch::~when_all_latch() = default;

auto when_all_latch::operator=(when_all_latch &&other) -> when_all_latch & {
    if(std::addressof(other) != this) {
        m_p->m_count.store(other.m_p->m_count.load(std::memory_order::acquire), std::memory_order::relaxed);
        m_p->m_awaiting_coroutine = std::exchange(other.m_p->m_awaiting_coroutine, nullptr);
    }

    return *this;
}

bool when_all_latch::is_ready() const noexcept {
    return m_p->m_awaiting_coroutine != nullptr && m_p->m_awaiting_coroutine.done();
}

bool when_all_latch::try_await(std::coroutine_handle<> awaiting_coroutine) noexcept {
    m_p->m_awaiting_coroutine = awaiting_coroutine;
    return m_p->m_count.fetch_sub(1, std::memory_order::acq_rel) > 1;
}

void when_all_latch::notify_awaitable_completed() noexcept {
    if(m_p->m_count.fetch_sub(1, std::memory_order::acq_rel) == 1) {
        m_p->m_awaiting_coroutine.resume();
    }
}

}
