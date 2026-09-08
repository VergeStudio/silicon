module;

#include <atomic>
#include <coroutine>
#include <map>
#include <memory>
#include <optional>
#include <utility>

export module silicon.scheduler:poll_info_impl;

import :fd;
import :poll;
import silicon.time;
import :poll_info;

export namespace silicon::scheduler {

struct poll_info_impl {

    fd_t m_fd{-1};

    poll_op m_op{};

    std::optional<poll_info::timed_events::iterator> m_timer_pos{std::nullopt};

    std::coroutine_handle<> m_awaiting_coroutine;

    poll_status m_poll_status{poll_status::error};

    bool m_processed{false};

    std::optional<poll_stop_token> m_cancel_trigger{std::nullopt};
};

}
