#include <silicon/test/test.hpp>

// 本 TU 自身定义协程（co_await），须直接可见 std::coroutine_traits，
// 不能只依赖 import silicon.coroutine。
#include <coroutine>
#include <expected>
#include <memory>
#include <atomic>
#include <vector>
#include <cstddef>
#include <utility>

import silicon.coroutine;

using silicon::coroutine::coroutine_pool;
using silicon::coroutine::sync_wait;
using silicon::scheduler::task;
using silicon::coroutine::when_all;
using silicon::scheduler::thread_pool;

namespace {

// 在 TEST_CASE 作用域内持有底层执行器（shared_ptr），确保其在 pool 析构后仍存活，
// 以便 worker / sender / async_close 协程能跑完，避免 use-after-free。
auto make_executor() -> std::shared_ptr<thread_pool> {
    return std::shared_ptr<thread_pool>{thread_pool::create().value()};
}

} // namespace

TEST_CASE("coroutine_pool: 基本分发并执行全部任务") {
    auto ex = make_executor();
    auto pool_result = coroutine_pool<thread_pool>::create(ex, 4);
    REQUIRE(pool_result.has_value());
    coroutine_pool<thread_pool> &pool = **pool_result;

    std::atomic<int> counter{0};
    auto driver = [&]() -> task<void> {
        for(int i = 0; i < 20; ++i) {
            pool.dispatch([&]() -> task<void> {
                counter.fetch_add(1, std::memory_order::relaxed);
                co_return;
            }());
        }
        co_await pool.join();
    };
    sync_wait(driver());

    CHECK(pool.empty());
    CHECK(counter.load() == 20);
}

TEST_CASE("coroutine_pool: 并发上限 = pool_size") {
    auto ex = make_executor();
    const std::size_t kPoolSize = 4;
    auto pool_result = coroutine_pool<thread_pool>::create(ex, kPoolSize);
    REQUIRE(pool_result.has_value());
    coroutine_pool<thread_pool> &pool = **pool_result;

    std::atomic<std::size_t> running{0};
    std::atomic<std::size_t> peak{0};
    auto driver = [&]() -> task<void> {
        for(int i = 0; i < 200; ++i) {
            pool.dispatch([&]() -> task<void> {
                auto cur = running.fetch_add(1, std::memory_order::acq_rel) + 1;
                // 记录并发峰值（CAS 循环更新）。
                auto p = peak.load(std::memory_order::relaxed);
                while(cur > p && !peak.compare_exchange_weak(p, cur, std::memory_order::acq_rel)) {}
                co_await ex->yield(); // 制造并发重叠窗口
                running.fetch_sub(1, std::memory_order::acq_rel);
            }());
        }
        co_await pool.join();
    };
    sync_wait(driver());

    // 任意时刻并发运行的任务数不超过 worker 数。
    CHECK(peak.load() <= kPoolSize);
    CHECK(pool.empty());
}

TEST_CASE("coroutine_pool: spawn_joinable 等待任务完成") {
    auto ex = make_executor();
    auto pool_result = coroutine_pool<thread_pool>::create(ex, 4);
    REQUIRE(pool_result.has_value());
    coroutine_pool<thread_pool> &pool = **pool_result;

    std::atomic<int> done{0};
    auto driver = [&]() -> task<void> {
        std::vector<task<void>> joins;
        joins.reserve(16);
        for(int i = 0; i < 16; ++i) {
            auto jt = pool.spawn_joinable([&]() -> task<void> {
                done.fetch_add(1, std::memory_order::relaxed);
                co_return;
            }());
            joins.push_back(std::move(jt));
        }
        // 等待全部 join 任务完成 => 全部用户任务已完成。
        co_await when_all(std::move(joins));
    };
    sync_wait(driver());

    CHECK(done.load() == 16);
    CHECK(pool.empty());
}

TEST_CASE("coroutine_pool: 析构时排空在途任务（不丢任务、不悬挂）") {
    auto ex = make_executor();
    std::atomic<int> counter{0};

    {
        auto pool_result = coroutine_pool<thread_pool>::create(ex, 4);
        REQUIRE(pool_result.has_value());
        coroutine_pool<thread_pool> &pool = **pool_result;
        auto driver = [&]() -> task<void> {
            for(int i = 0; i < 30; ++i) {
                pool.dispatch([&]() -> task<void> {
                    counter.fetch_add(1, std::memory_order::relaxed);
                    co_return;
                }());
            }
            // 故意不显式 join：依赖析构自旋 + async_close 排空。
        };
        sync_wait(driver());
        // driver 返回后 worker 可能仍在处理/排队；离开作用域触发析构。
    }

    // async_close 会等待所有待发 sender 入队后再关通道，故不应丢失任何任务。
    CHECK(counter.load() == 30);
}

TEST_CASE("coroutine_pool: shutdown 后拒绝新任务且仍排空已入队任务") {
    auto ex = make_executor();
    auto pool_result = coroutine_pool<thread_pool>::create(ex, 4);
    REQUIRE(pool_result.has_value());
    coroutine_pool<thread_pool> &pool = **pool_result;

    std::atomic<int> counter{0};
    auto driver = [&]() -> task<void> {
        for(int i = 0; i < 10; ++i) {
            pool.dispatch([&]() -> task<void> {
                counter.fetch_add(1, std::memory_order::relaxed);
                co_return;
            }());
        }
        co_await pool.join();
        // join 后全部完成；再 dispatch 应被拒绝（shutdown 已调用）。
        pool.shutdown();
        bool rejected = !pool.dispatch([&]() -> task<void> {
            counter.fetch_add(1000, std::memory_order::relaxed); // 不应执行
            co_return;
        }());
        CHECK(rejected);
    };
    sync_wait(driver());

    CHECK(counter.load() == 10);
    CHECK(pool.empty());
}
