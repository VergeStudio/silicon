#pragma once

#if defined(_WIN32)
#    include "silicon/coroutine/detail/io_notifier_iocp.hpp"
#elif defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__)
#    include "silicon/coroutine/detail/io_notifier_kqueue.hpp"
#elif defined(__linux__)
#    include "silicon/coroutine/detail/io_notifier_epoll.hpp"
#endif

namespace silicon::coroutine {

#if defined(_WIN32)
using io_notifier = detail::io_notifier_iocp;
#elif defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__)
using io_notifier = detail::io_notifier_kqueue;
#elif defined(__linux__)
using io_notifier = detail::io_notifier_epoll;
#endif

} // namespace silicon::coroutine
