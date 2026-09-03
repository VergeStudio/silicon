#pragma once

// Full definition of poll_info::impl.
//
// MSVC limitation (reproduced in a minimal experiment): non-exported entities
// of an exported partition are invisible to implementation units, and even an
// `export struct poll_info::impl` out-of-line nested-class definition inside a
// partition is NOT visible to `module silicon.scheduler;` implementation units.
// Entities text-included into an implementation unit's purview ARE visible, so
// the definition lives in this textual header, included by each scheduler .cpp
// after its module/import declarations (project PIMPL convention, cf.
// core/shared_library_impl.h).
//
// This header must NOT #include standard headers: a purview #include of a std
// header re-declares it and clashes with the std declarations carried by the
// module IFCs (C2953). The required standard headers (<coroutine>, <map>,
// <optional>) are provided by each implementation unit's global module
// fragment. Module entities (fd_t/poll_op/poll_status/poll_stop_token) are
// qualified with their defining namespace so no alias dependency is needed.
namespace silicon::scheduler {
struct poll_info::impl {
    silicon::coroutine::fd_t m_fd{-1};
    silicon::coroutine::poll_op m_op{};
    std::optional<poll_info::timed_events::iterator> m_timer_pos{std::nullopt};
    std::coroutine_handle<> m_awaiting_coroutine;
    silicon::coroutine::poll_status m_poll_status{silicon::coroutine::poll_status::error};
    bool m_processed{false};
    std::optional<silicon::coroutine::poll_stop_token> m_cancel_trigger{std::nullopt};
};
} // namespace silicon::scheduler
