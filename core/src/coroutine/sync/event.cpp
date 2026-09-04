module;

#include <atomic>
#include <memory>

module silicon.coroutine;



namespace silicon::coroutine {

struct event::impl {
  public:






    mutable std::atomic<void *> m_state;
};

event::event(bool initially_set) noexcept: m_p(std::make_unique<impl>()) {
    m_p->m_state.store((initially_set) ? static_cast<void *>(this) : nullptr, std::memory_order::relaxed);
}

event::~event() = default;

bool event::is_set() const noexcept {
    return m_p->m_state.load(std::memory_order::acquire) == this;
}

void * event::exchange_set_state() noexcept {
    return m_p->m_state.exchange(this, std::memory_order::acq_rel);
}

void event::set(resume_order_policy policy) noexcept {


    void *old_value = m_p->m_state.exchange(this, std::memory_order::acq_rel);
    if(old_value != this) {

        if(policy == resume_order_policy::kFifo) {
            old_value = reverse(static_cast<awaiter *>(old_value));
        }


        auto *waiters = static_cast<awaiter *>(old_value);
        while(waiters != nullptr) {
            auto *next = waiters->m_next;
            waiters->m_awaiting_coroutine.resume();
            waiters = next;
        }
    }
}

auto event::reverse(awaiter *curr) -> awaiter * {
    return silicon::scheduler::awaiter_list_reverse(curr);
}

bool event::awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    const void *const set_state = &m_event;

    m_awaiting_coroutine = awaiting_coroutine;


    void *old_value = m_event.m_p->m_state.load(std::memory_order::acquire);
    do {

        if(old_value == set_state) {
            return false;
        }

        m_next = static_cast<awaiter *>(old_value);
    } while(!m_event.m_p->m_state.compare_exchange_weak(
            old_value, this, std::memory_order::release, std::memory_order::acquire
    ));

    return true;
}

void event::reset() noexcept {
    void *old_value = this;
    m_p->m_state.compare_exchange_strong(old_value, nullptr, std::memory_order::acquire);
}

}
