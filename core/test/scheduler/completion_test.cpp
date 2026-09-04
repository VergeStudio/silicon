#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <system_error>
#include <utility>

#if !defined(SILICON_PLATFORM_WINDOWS)
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <silicon/test/test.h>

import silicon.scheduler;

namespace sched = silicon::scheduler;
namespace coro = silicon::scheduler;

namespace {

struct temp_regular_file {
    std::FILE *m_file{nullptr};
    int m_fd{-1};

    static temp_regular_file create() {
        temp_regular_file tf;
        tf.m_file = std::tmpfile();
        if(tf.m_file == nullptr) { return tf; }
#if defined(SILICON_PLATFORM_WINDOWS)
        tf.m_fd = ::_fileno(tf.m_file);
#else
        tf.m_fd = ::fileno(tf.m_file);
#endif
        return tf;
    }

    bool valid() const noexcept { return m_file != nullptr && m_fd >= 0; }

    bool write_byte(char value) {
        if(m_file == nullptr) { return false; }
        const bool ok = std::fwrite(&value, 1, 1, m_file) == 1 && std::fflush(m_file) == 0;
        return ok;
    }

    ~temp_regular_file() {
        if(m_file != nullptr) { std::fclose(m_file); }
    }
};

auto read_at_result(
        sched::io_scheduler &ios, int fd, void *buffer, std::uint32_t length, std::uint64_t offset
) -> sched::task<sched::result<int64_t>> {
    co_return co_await ios.read_at(fd, buffer, length, offset);
}

auto write_at_result(
        sched::io_scheduler &ios, int fd, const void *buffer, std::uint32_t length, std::uint64_t offset
) -> sched::task<sched::result<int64_t>> {
    co_return co_await ios.write_at(fd, buffer, length, offset);
}

auto make_io_scheduler() -> sched::result<std::unique_ptr<sched::io_scheduler>> {
    return sched::io_scheduler::create(sched::io_scheduler::options{
        .thread_strategy = sched::io_scheduler::thread_strategy_t::spawn,
        .pool = {.thread_count = 1},
        .execution_strategy =
            sched::io_scheduler::execution_strategy_t::process_tasks_on_thread_pool});
}

}

TEST_CASE("completion I/O：无后端时常规文件 read_at/write_at 按契约返回 kNoCompletionBackend") {
    auto created = make_io_scheduler();
    REQUIRE(created.has_value());
    auto &ios = *created.value();
    CHECK(ios.is_shutdown() == false);

    auto file = temp_regular_file::create();
    REQUIRE(file.valid());
    REQUIRE(file.write_byte('A'));

    char read_buffer[1]{};
    const char write_buffer[1]{'B'};

    auto read_probe = coro::sync_wait(read_at_result(ios, file.m_fd, read_buffer, 1, 0));
    auto write_probe = coro::sync_wait(write_at_result(ios, file.m_fd, write_buffer, 1, 0));

    if(ios.completion_backend() == sched::io_ring::backend::none) {

        REQUIRE_FALSE(read_probe.has_value());
        CHECK(read_probe.error() == sched::make_error_code(sched::scheduler_error::kNoCompletionBackend));
        REQUIRE_FALSE(write_probe.has_value());
        CHECK(write_probe.error() == sched::make_error_code(sched::scheduler_error::kNoCompletionBackend));
    }

    ios.shutdown();
}

TEST_CASE("completion I/O：文件预检拒绝管道 fd（read_at/write_at → kNotRegularFile）") {
    auto created = make_io_scheduler();
    REQUIRE(created.has_value());
    auto &ios = *created.value();

    auto pipe = coro::pipe_t::create();
    REQUIRE(pipe.has_value());

    char buffer[8]{};
    const char payload[8]{"abcdefg"};

    auto read_result = coro::sync_wait(read_at_result(ios, pipe->read_fd(), buffer, 1, 0));
    REQUIRE_FALSE(read_result.has_value());
    CHECK(read_result.error() == sched::make_error_code(sched::scheduler_error::kNotRegularFile));

    auto write_result = coro::sync_wait(write_at_result(ios, pipe->write_fd(), payload, 1, 0));
    REQUIRE_FALSE(write_result.has_value());
    CHECK(write_result.error() == sched::make_error_code(sched::scheduler_error::kNotRegularFile));

    ios.shutdown();
}

#if !defined(SILICON_PLATFORM_WINDOWS)
TEST_CASE("completion I/O：文件预检拒绝 socket fd（POSIX socketpair → kNotRegularFile）") {
    auto created = make_io_scheduler();
    REQUIRE(created.has_value());
    auto &ios = *created.value();

    int sockets[2]{-1, -1};
    REQUIRE(::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);

    char buffer[8]{};
    const char payload[8]{"abcdefg"};

    auto read_result = coro::sync_wait(read_at_result(ios, sockets[0], buffer, 1, 0));
    REQUIRE_FALSE(read_result.has_value());
    CHECK(read_result.error() == sched::make_error_code(sched::scheduler_error::kNotRegularFile));

    auto write_result = coro::sync_wait(write_at_result(ios, sockets[1], payload, 1, 0));
    REQUIRE_FALSE(write_result.has_value());
    CHECK(write_result.error() == sched::make_error_code(sched::scheduler_error::kNotRegularFile));

    ::close(sockets[0]);
    ::close(sockets[1]);
    ios.shutdown();
}
#endif

#if defined(SILICON_PLATFORM_LINUX)
TEST_CASE("completion I/O：后端可用时对常规文件完成一次读往返") {
    auto created = make_io_scheduler();
    REQUIRE(created.has_value());
    auto &ios = *created.value();

    auto file = temp_regular_file::create();
    REQUIRE(file.valid());
    REQUIRE(file.write_byte('A'));

    char probe_buffer[1]{};
    auto probe = coro::sync_wait(read_at_result(ios, file.m_fd, probe_buffer, 1, 0));
    if(ios.completion_backend() == sched::io_ring::backend::none) {
        ios.shutdown();
        return;
    }

    REQUIRE(probe.has_value());
    CHECK(probe.value() == 1);
    CHECK(probe_buffer[0] == 'A');

    ios.shutdown();
}
#endif
