export module silicon.coroutine:io_notifier;

// 平台专属 io_notifier 分区按平台条件 import：
// 对应源文件在 xmake.lua 中按平台 remove_files，非本平台的分区 BMI 不存在。
#if defined(_WIN32)
import :detail.io_notifier_iocp;
#elif defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__)
import :detail.io_notifier_kqueue;
#elif defined(__linux__)
import :detail.io_notifier_epoll;
#endif

export namespace silicon::coroutine {

#if defined(_WIN32)
using io_notifier = detail::io_notifier_iocp;
#elif defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__)
using io_notifier = detail::io_notifier_kqueue;
#elif defined(__linux__)
using io_notifier = detail::io_notifier_epoll;
#endif

} // namespace silicon::coroutine
