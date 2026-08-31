module;

#include <atomic>
#include <memory>

module silicon.coroutine;



namespace silicon::coroutine {

struct event::impl {
  public:
    /// The state of the event, nullptr is not set with zero awaiters.  Set to an awaiter* there
    /// are coroutines awaiting the event to be set, and set to the owning event the event has
    /// triggered.
    /// 1) nullptr == not set
    /// 2) awaiter* == linked list of awaiters waiting for the event to trigger.
    /// 3) &event == The event is triggered and all awaiters are resumed.
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
    // Exchange the state to this, if the state was previously not this, then traverse the list
    // of awaiters and resume their coroutines.
    void *old_value = m_p->m_state.exchange(this, std::memory_order::acq_rel);
    if(old_value != this) {
        // If FIFO has been requsted then reverse the order upon resuming.
        if(policy == resume_order_policy::kFifo) {
            old_value = reverse(static_cast<awaiter *>(old_value));
        }
        // else lifo nothing to do

        auto *waiters = static_cast<awaiter *>(old_value);
        while(waiters != nullptr) {
            auto *next = waiters->m_next;
            waiters->m_awaiting_coroutine.resume();
            waiters = next;
        }
    }
}

auto event::reverse(awaiter *curr) -> awaiter * {
    return awaiter_list_reverse(curr);
}

bool event::awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    const void *const set_state = &m_event;

    m_awaiting_coroutine = awaiting_coroutine;

    // This value will update if other threads write to it via acquire.
    void *old_value = m_event.m_p->m_state.load(std::memory_order::acquire);
    do {
        // Resume immediately if already in the set state.
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

} // namespace silicon::coroutine
