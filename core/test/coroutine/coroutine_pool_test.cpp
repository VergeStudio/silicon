#include <silicon/test/test.h>



#include <chrono>
#include <coroutine>
#include <thread>
#include <expected>
#include <memory>
#include <atomic>
#include <vector>
#include <cstddef>
#include <utility>

import silicon.coroutine;

using silicon::coroutine::coroutine_pool;
using silicon::scheduler::sync_wait;
using silicon::scheduler::task;
using silicon::coroutine::when_all;
using silicon::scheduler::thread_pool;

namespace {



std::shared_ptr<thread_pool> make_executor() {
    return std::shared_ptr<thread_pool>{thread_pool::create().value()};
}





auto make_counter_task(std::atomic<int> *counter) -> task<void> {
    counter->fetch_add(1, std::memory_order::relaxed);
    co_return;
}

auto make_concurrency_task(std::atomic<std::size_t> *running, std::atomic<std::size_t> *peak,
                           std::shared_ptr<thread_pool> ex) -> task<void> {
    auto cur = running->fetch_add(1, std::memory_order::acq_rel) + 1;

    auto p = peak->load(std::memory_order::relaxed);
    while(cur > p && !peak->compare_exchange_weak(p, cur, std::memory_order::acq_rel)) {}
    co_await ex->yield();
    running->fetch_sub(1, std::memory_order::acq_rel);
}

auto make_done_task(std::atomic<int> *done) -> task<void> {
    done->fetch_add(1, std::memory_order::relaxed);
    co_return;
}

auto make_rejected_task(std::atomic<int> *counter) -> task<void> {
    counter->fetch_add(1000, std::memory_order::relaxed);
    co_return;
}

}

TEST_CASE("coroutine_pool: 基本分发并执行全部任务") {
    auto ex = make_executor();
    auto pool_result = coroutine_pool<thread_pool>::create(ex, 4);
    REQUIRE(pool_result.has_value());
    coroutine_pool<thread_pool> &pool = **pool_result;

    std::atomic<int> counter{0};
    auto driver = [&]() -> task<void> {
        for(int i = 0; i < 20; ++i) {
            pool.dispatch(make_counter_task(&counter));
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
            pool.dispatch(make_concurrency_task(&running, &peak, ex));
        }
        co_await pool.join();
    };
    sync_wait(driver());


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
            auto jt = pool.spawn_joinable(make_done_task(&done));
            joins.push_back(std::move(jt));
        }

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
                pool.dispatch(make_counter_task(&counter));
            }



            co_return;
        };
        sync_wait(driver());

    }


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
            pool.dispatch(make_counter_task(&counter));
        }
        co_await pool.join();

        pool.shutdown();
        bool rejected = !pool.dispatch(make_rejected_task(&counter));
        CHECK(rejected);
    };
    sync_wait(driver());

    CHECK(counter.load() == 10);
    CHECK(pool.empty());
}
