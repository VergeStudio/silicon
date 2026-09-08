module;

#if defined(SILICON_PLATFORM_LINUX)
#    include <cerrno>
#endif

#include <memory>
#include <system_error>

#include <silicon/common.h>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_scheduler;
import :pipe;

#if defined(SILICON_PLATFORM_LINUX)

namespace silicon::scheduler {

// completion 引擎的 Linux 平台接缝：错误码映射与基于 pipe 的唤醒通道。
// Windows 实现见 io_scheduler_completion_win.cpp，file_is_regular 见 _unix.cpp。

std::error_code completion_result_to_error(std::int64_t result) {
    return std::error_code(static_cast<int>(-result), std::generic_category());
}

class completion_wake::impl {
  public:
    pipe_t m_pipe{};
    bool m_registered{false};
};

completion_wake::completion_wake(): m_p(std::make_unique<impl>()) {}

completion_wake::~completion_wake() = default;

bool completion_wake::setup(io_notifier &notifier, void *sentinel) {
    auto created = pipe_t::create();
    if(!created) { return false; }
    m_p->m_pipe = std::move(*created);

    if(!notifier.watch(m_p->m_pipe.read_fd(), poll_op::read, sentinel, true, false)) {
        m_p->m_pipe.close();
        return false;
    }
    m_p->m_registered = true;
    return true;
}

void completion_wake::teardown(io_notifier &notifier) {
    if(m_p->m_registered) {
        notifier.unwatch(m_p->m_pipe.read_fd(), poll_op::read);
        m_p->m_registered = false;
    }
    m_p->m_pipe.close();
}

void completion_wake::drain() noexcept {
    if(!m_p->m_pipe.is_valid()) { return; }
    char buffer[64]{};
    while(true) {
        const long n = m_p->m_pipe.read(buffer, sizeof(buffer));
        if(n > 0) { continue; }

        if(n < 0 && errno == EAGAIN) { break; }
        break;
    }
}

void completion_wake::notify(io_notifier &notifier, void *sentinel) noexcept {
    (void)notifier;
    (void)sentinel;
    if(!m_p->m_pipe.is_valid()) { return; }
    const char byte = 1;
    const long written = m_p->m_pipe.write(&byte, sizeof(byte));
    if(written != static_cast<long>(sizeof(byte))) {

    }
}

}

#endif
