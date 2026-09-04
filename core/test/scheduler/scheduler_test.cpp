// scheduler 模块测试：覆盖 task / sync_wait / task_event / task_group、四种调度器
// （inline / run_loop / thread_pool / parallel）、类型擦除门面 scheduler_facade，
// 以及调度原语（错误码 / pipe / awaiter_list）。
#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

#include <silicon/test/test.h>

import silicon.proxy;
import silicon.scheduler;

namespace sched = silicon::scheduler;
namespace coro = silicon::scheduler;

namespace {

// —— 测试用协程 ——
// task<T>（T 非引用）的返回值存放在协程帧内，sync_wait 的返回类型是引用
// （awaiter_return_type 为 const T& / T&&），会在 sync_wait_task 析构、协程帧
// 销毁后悬空。故取值一律用 resume() + promise().result()（task 对象存活期内读取），
// sync_wait 只用于 void 返回与按值返回的 awaitable。

auto make_value_task(int v) -> sched::task<int> {
    co_return v;
}

auto make_throwing_task() -> sched::task<int> {
    throw std::runtime_error("value task failure");
    co_return 1;
}

auto set_value_task(int &out, int v) -> sched::task<void> {
    out = v;
    co_return;
}

auto make_throwing_void_task() -> sched::task<void> {
    throw std::runtime_error("void task failure");
    co_return;
}

auto increment_task(std::atomic<int> &counter) -> sched::task<void> {
    counter.fetch_add(1, std::memory_order::relaxed);
    co_return;
}

auto capture_thread_id_task(std::thread::id &out) -> sched::task<void> {
    out = std::this_thread::get_id();
    co_return;
}

template <typename executor_type>
auto wait_group(sched::task_group<executor_type> &group) -> sched::task<void> {
    co_await group;
    co_return;
}

auto await_event(const sched::task_event &event, bool &reached) -> sched::task<void> {
    co_await event;
    reached = true;
    co_return;
}

auto await_delay(sched::io_scheduler &ios, std::chrono::milliseconds delay, bool &resumed)
    -> sched::task<void> {
    co_await ios.schedule_after(delay);
    resumed = true;
    co_return;
}

// 按值返回的 awaitable：用于覆盖 sync_wait 的按值返回分支（返回类型为 int，
// 与协程帧内存储的引用型返回区分开）。
struct value_awaiter {
    int value;

    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) noexcept {}
    int await_resume() const noexcept { return value; }
};

// awaiter 单向链表的元素类型：须含 m_next 与 m_awaiting_coroutine 两个成员。
struct list_entry {
    std::coroutine_handle<> m_awaiting_coroutine{};
    list_entry *m_next{nullptr};
};

} // namespace

TEST_CASE("scheduler 错误码走 silicon.scheduler category") {
    const std::error_code ec = sched::make_error_code(sched::scheduler_error::kShuttingDown);
    CHECK(ec.value() == static_cast<int>(sched::scheduler_error::kShuttingDown));
    CHECK(std::string(ec.category().name()) == "silicon.scheduler");
    CHECK(std::string(ec.message()) == "scheduler is shutting down");

    const std::error_code unknown = sched::make_error_code(sched::scheduler_error::kUnknown);
    CHECK(std::string(unknown.message()) == "unknown scheduler error");
    CHECK(unknown != ec);
}

TEST_CASE("task<T>：默认构造为空，resume 后取到返回值") {
    sched::task<int> empty;
    CHECK(empty.is_ready());
    CHECK(empty.destroy() == false);

    auto t = make_value_task(7);
    CHECK(t.is_ready() == false);
    CHECK(t.resume() == false); // 协程体内无挂起点，一次 resume 跑到结束
    CHECK(t.is_ready());
    CHECK(t.promise().result() == 7);
}

TEST_CASE("task<T>：移动语义转移协程所有权") {
    auto t = make_value_task(5);
    sched::task<int> moved = std::move(t);
    CHECK(t.handle() == nullptr);
    CHECK(moved.is_ready() == false);

    CHECK(moved.resume() == false);
    CHECK(moved.promise().result() == 5);
    CHECK(moved.destroy());
    CHECK(moved.is_ready());
}

TEST_CASE("task<T>：协程内异常经 result() 重新抛出") {
    auto t = make_throwing_task();
    CHECK(t.resume() == false);
    CHECK_THROWS_AS(t.promise().result(), std::runtime_error);
}

TEST_CASE("sync_wait：驱动 void 协程并在返回前等待其完成") {
    int out = 0;
    coro::sync_wait(set_value_task(out, 99));
    CHECK(out == 99);
}

TEST_CASE("sync_wait：取回按值返回的 awaitable 结果") {
    CHECK(coro::sync_wait(value_awaiter{42}) == 42);
}

TEST_CASE("sync_wait：传播协程内抛出的异常") {
    CHECK_THROWS_AS(coro::sync_wait(make_throwing_void_task()), std::runtime_error);
}

TEST_CASE("task_event：set / reset 与置位后 co_await 立即返回") {
    sched::task_event event;
    CHECK(event.is_set() == false);

    event.set();
    CHECK(event.is_set());

    event.reset();
    CHECK(event.is_set() == false);

    event.set();
    bool reached = false;
    coro::sync_wait(await_event(event, reached));
    CHECK(reached);
}

TEST_CASE("inline_scheduler：在当前线程同步执行提交的任务") {
    sched::inline_scheduler s;
    CHECK(s.is_shutdown() == false);
    CHECK(s.empty());
    CHECK(s.size() == 0);

    int value = 0;
    CHECK(s.spawn_detached(set_value_task(value, 5)));
    CHECK(value == 5); // 内联执行：spawn 返回前任务已经跑完
    CHECK(s.empty());
    CHECK(s.size() == 0);

    s.shutdown();
    CHECK(s.is_shutdown());
    CHECK(s.spawn_detached(set_value_task(value, 9)) == false);
    CHECK(value == 5);
}

TEST_CASE("inline_scheduler：spawn_joinable 返回可等待句柄") {
    sched::inline_scheduler s;
    int value = 0;
    auto joined = s.spawn_joinable(set_value_task(value, 11));
    coro::sync_wait(std::move(joined));
    CHECK(value == 11);
}

TEST_CASE("run_loop：finish 后 run() 排空队列再返回") {
    sched::run_loop loop;
    CHECK(loop.is_shutdown() == false);

    int a = 0;
    int b = 0;
    int c = 0;
    CHECK(loop.spawn_detached(set_value_task(a, 1)));
    CHECK(loop.spawn_detached(set_value_task(b, 2)));
    CHECK(loop.spawn_detached(set_value_task(c, 3)));

    CHECK(loop.empty() == false);
    CHECK(a == 0); // 未调用 run()，入队的任务尚未执行

    loop.finish();
    CHECK(loop.is_shutdown());

    loop.run();
    CHECK(a == 1);
    CHECK(b == 2);
    CHECK(c == 3);
    CHECK(loop.size() == 0);
    CHECK(loop.empty());
}

TEST_CASE("thread_pool：创建成功并并发执行提交的任务") {
    auto pool = sched::thread_pool::create(sched::thread_pool::options{.thread_count = 2});
    REQUIRE(pool.has_value());
    auto &tp = *pool;

    CHECK(tp->thread_count() == 2);
    CHECK(tp->is_shutdown() == false);

    std::atomic<int> counter{0};
    {
        sched::task_group<sched::thread_pool> group(pool.value());
        for(int i = 0; i < 8; ++i) {
            CHECK(group.start(increment_task(counter)));
        }
        // group.size() 在并发下是竞态值：任务可能已被工作线程跑完并计数归零，
        // 故只断言 sync_wait 之后的稳定状态。
        coro::sync_wait(wait_group(group));
        CHECK(counter.load() == 8);
        CHECK(group.empty());
    }

    int value = 0;
    auto joined = tp->spawn_joinable(set_value_task(value, 42));
    coro::sync_wait(std::move(joined));
    CHECK(value == 42);

    std::thread::id worker{};
    coro::sync_wait(tp->schedule(capture_thread_id_task(worker)));
    CHECK(worker != std::thread::id{});

    tp->shutdown();
    CHECK(tp->is_shutdown());
    CHECK(tp->spawn_detached(set_value_task(value, 7)) == false);
}

TEST_CASE("parallel_scheduler：get_parallel_scheduler 返回进程唯一实例") {
    auto &first = sched::parallel_scheduler::get_parallel_scheduler();
    auto &second = sched::parallel_scheduler::get_parallel_scheduler();
    CHECK(&first == &second);
    CHECK(first.thread_count() > 0);
}

TEST_CASE("parallel_scheduler：本地实例满足 scheduler_facade 语义") {
    sched::parallel_scheduler ps;
    CHECK(ps.thread_count() > 0);
    CHECK(ps.is_shutdown() == false);

    std::atomic<int> counter{0};
    auto joined = ps.spawn_joinable(increment_task(counter));
    coro::sync_wait(std::move(joined));
    CHECK(counter.load() == 1);

    ps.shutdown();
    CHECK(ps.is_shutdown());
}

TEST_CASE("scheduler_facade：类型擦除后统一驱动调度器") {
    auto proxy = sched::make_scheduler<sched::inline_scheduler>();
    CHECK(proxy.has_value());

    int value = 0;
    CHECK(proxy->spawn_detached(set_value_task(value, 3)));
    CHECK(value == 3);
    CHECK(proxy->size() == 0);
    CHECK(proxy->empty());
    CHECK(proxy->is_shutdown() == false);

    proxy->shutdown();
    CHECK(proxy->is_shutdown());
    CHECK(proxy->spawn_detached(set_value_task(value, 4)) == false);
}

TEST_CASE("scheduler_view：以非拥有视图擦除既有调度器") {
    auto pool = sched::thread_pool::create(sched::thread_pool::options{.thread_count = 2});
    REQUIRE(pool.has_value());

    sched::scheduler_view view =
        silicon::proxy::make_proxy_view<sched::scheduler_facade>(*pool.value());
    CHECK(view.has_value());

    std::atomic<int> counter{0};
    CHECK(view->resume(sched::task<void>{}.handle()) == false); // 空句柄不入队
    auto joined = view->spawn_joinable(increment_task(counter));
    coro::sync_wait(std::move(joined));
    CHECK(counter.load() == 1);

    view->shutdown();
    CHECK(view->is_shutdown());
}

TEST_CASE("io_scheduler：spawn 模式下提交的任务被执行完成") {
    auto created = sched::io_scheduler::create(sched::io_scheduler::options{
        .thread_strategy = sched::io_scheduler::thread_strategy_t::spawn,
        .pool = {.thread_count = 1},
        .execution_strategy =
            sched::io_scheduler::execution_strategy_t::process_tasks_on_thread_pool});
    REQUIRE(created.has_value());
    auto &ios = *created.value();
    CHECK(ios.is_shutdown() == false);

    std::atomic<int> counter{0};
    {
        sched::task_group<sched::io_scheduler> group(created.value());
        for(int i = 0; i < 4; ++i) {
            CHECK(group.start(increment_task(counter)));
        }
        coro::sync_wait(wait_group(group));
        CHECK(counter.load() == 4);
        CHECK(group.empty());
    }

    CHECK(ios.size() == 0);

    ios.shutdown();
    CHECK(ios.is_shutdown());
    CHECK(ios.spawn_detached(increment_task(counter)) == false);
}

TEST_CASE("io_scheduler：schedule_after 在到期后恢复协程") {
    auto created = sched::io_scheduler::create(sched::io_scheduler::options{
        .thread_strategy = sched::io_scheduler::thread_strategy_t::spawn,
        .pool = {.thread_count = 1},
        .execution_strategy =
            sched::io_scheduler::execution_strategy_t::process_tasks_on_thread_pool});
    REQUIRE(created.has_value());
    auto &ios = *created.value();

    const auto before = std::chrono::steady_clock::now();
    bool resumed = false;
    coro::sync_wait(await_delay(ios, std::chrono::milliseconds{50}, resumed));
    const auto elapsed = std::chrono::steady_clock::now() - before;

    CHECK(resumed);
    // 到期时间按毫秒截断，定时器不会提前触发：只断言「不早于请求时长的一半」，
    // 避免把调度抖动误判为失败。
    CHECK(elapsed >= std::chrono::milliseconds{25});

    ios.shutdown();
}

TEST_CASE("io_scheduler：manual 模式下 process_events 驱动定时恢复") {
    auto created = sched::io_scheduler::create(sched::io_scheduler::options{
        .thread_strategy = sched::io_scheduler::thread_strategy_t::manual,
        .pool = {.thread_count = 1},
        .execution_strategy =
            sched::io_scheduler::execution_strategy_t::process_tasks_on_thread_pool});
    REQUIRE(created.has_value());
    auto &ios = *created.value();
    CHECK(ios.is_shutdown() == false);

    // manual 模式无后台事件循环线程，须由调用方反复 process_events 驱动定时器。
    bool resumed = false;
    auto task = ios.spawn_joinable(await_delay(ios, std::chrono::milliseconds{20}, resumed));
    for(int i = 0; i < 50 && !resumed; ++i) {
        ios.process_events(std::chrono::milliseconds{10});
    }
    REQUIRE(resumed);

    coro::sync_wait(std::move(task));
    ios.shutdown();
    CHECK(ios.is_shutdown());
}

TEST_CASE("pipe_t：创建成功并完成一次读写往返") {
    auto created = coro::pipe_t::create();
    REQUIRE(created.has_value());

    auto &pipe = created.value();
    CHECK(pipe.is_valid());
    CHECK(pipe.read_fd() != pipe.write_fd());

    const char payload[] = "abcd";
    CHECK(pipe.write(payload, sizeof(payload)) == static_cast<long>(sizeof(payload)));

    char buffer[4]{};
    CHECK(pipe.read(buffer, sizeof(buffer)) == static_cast<long>(sizeof(buffer)));
    CHECK(std::string(buffer, sizeof(buffer)) == "abcd");
}

TEST_CASE("awaiter_list：push 后按后进先出顺序 pop") {
    std::atomic<list_entry *> list{nullptr};
    list_entry a;
    list_entry b;

    coro::awaiter_list_push(list, &a);
    coro::awaiter_list_push(list, &b);

    CHECK(coro::awaiter_list_pop(list) == &b);
    CHECK(coro::awaiter_list_pop(list) == &a);
    CHECK(coro::awaiter_list_pop(list) == nullptr);
}

TEST_CASE("awaiter_list：pop_all 摘取整条链表且可反转") {
    std::atomic<list_entry *> list{nullptr};
    list_entry a;
    list_entry b;
    list_entry c;

    coro::awaiter_list_push(list, &a);
    coro::awaiter_list_push(list, &b);
    coro::awaiter_list_push(list, &c);

    auto *head = coro::awaiter_list_pop_all(list);
    CHECK(head == &c);
    CHECK(head->m_next == &b);
    CHECK(head->m_next->m_next == &a);
    CHECK(head->m_next->m_next->m_next == nullptr);
    CHECK(coro::awaiter_list_pop_all(list) == nullptr);

    auto *reversed = coro::awaiter_list_reverse(head);
    CHECK(reversed == &a);
    CHECK(reversed->m_next == &b);
    CHECK(reversed->m_next->m_next == &c);
    CHECK(reversed->m_next->m_next->m_next == nullptr);
}

TEST_CASE("io_scheduler：completion read_at 默认契约（无后端时立即降级）") {
    // completion 引擎（io_ring）是 readiness 之外的独立引擎：未编译后端或后端
    // 初始化失败时，read_at/write_at 对常规文件必须立即返回 kNoCompletionBackend，
    // 不得挂起、不得阻塞，调度器其余能力不受影响。
    auto created = sched::io_scheduler::create(sched::io_scheduler::options{
        .thread_strategy = sched::io_scheduler::thread_strategy_t::spawn,
        .pool = {.thread_count = 1},
        .execution_strategy =
            sched::io_scheduler::execution_strategy_t::process_tasks_on_thread_pool});
    REQUIRE(created.has_value());
    auto &ios = *created.value();

    // 常规文件 fd（跨平台：CRT tmpfile + fileno/_fileno）。
    struct temp_file {
        std::FILE *file{nullptr};
        int fd{-1};

        bool write_byte(char value) {
            return file != nullptr && std::fwrite(&value, 1, 1, file) == 1 && std::fflush(file) == 0;
        }

        ~temp_file() {
            if(file != nullptr) { std::fclose(file); }
        }
    };
    temp_file tf;
    tf.file = std::tmpfile();
    REQUIRE(tf.file != nullptr);
#if defined(SILICON_PLATFORM_WINDOWS)
    tf.fd = ::_fileno(tf.file);
#else
    tf.fd = ::fileno(tf.file);
#endif
    REQUIRE(tf.fd >= 0);
    REQUIRE(tf.write_byte('X'));

    // 先跑一次真实 read_at 触发惰性探测（后端可用则完成一次读）。
    char buffer[1]{};
    auto read_probe = coro::sync_wait(ios.read_at(tf.fd, buffer, 1, 0));

    // 仅在确实无后端时断言默认降级契约；有后端（io_ring=y 的 Linux/Windows CI）
    // 时 read_at 走真路径，该契约不适用。
    if(ios.completion_backend() == sched::io_ring::backend::none) {
        REQUIRE_FALSE(read_probe.has_value());
        CHECK(read_probe.error() == sched::make_error_code(sched::scheduler_error::kNoCompletionBackend));
    }

    ios.shutdown();
}
