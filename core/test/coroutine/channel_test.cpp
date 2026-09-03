#include <silicon/test/test.h>

// 本 TU 自身定义协程（co_await），须直接可见 std::coroutine_traits，
// 不能只依赖 import silicon.coroutine。
#include <coroutine>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

import silicon.coroutine;

using silicon::coroutine::channel;
using silicon::coroutine::sync_wait;
using silicon::scheduler::task;
using silicon::coroutine::when_all;

namespace cr = silicon::coroutine::channel_result;

TEST_CASE("channel: 有缓冲 FIFO 顺序") {
    channel<int> ch{4};
    CHECK(ch.capacity() == 4);
    CHECK(ch.empty());
    CHECK(!ch.full());

    CHECK(sync_wait(ch.send(1)) == cr::send::kSent);
    CHECK(sync_wait(ch.send(2)) == cr::send::kSent);
    CHECK(sync_wait(ch.send(3)) == cr::send::kSent);
    CHECK(ch.size() == 3);
    CHECK(!ch.full());

    auto a = sync_wait(ch.recv());
    REQUIRE(a.has_value());
    CHECK(*a == 1);
    auto b = sync_wait(ch.recv());
    REQUIRE(b.has_value());
    CHECK(*b == 2);
    auto c = sync_wait(ch.recv());
    REQUIRE(c.has_value());
    CHECK(*c == 3);
    CHECK(ch.empty());
}

TEST_CASE("channel: 满时 send 挂起，消费者腾槽后按 FIFO 入槽") {
    channel<int> ch{1};
    CHECK(sync_wait(ch.send(1)) == cr::send::kSent);
    CHECK(ch.full());

    std::vector<int> produced;
    auto sender_a = [&]() -> task<void> {
        CHECK(co_await ch.send(2) == cr::send::kSent);
        produced.push_back(2);
    };
    auto sender_b = [&]() -> task<void> {
        CHECK(co_await ch.send(3) == cr::send::kSent);
        produced.push_back(3);
    };
    auto consumer = [&]() -> task<void> {
        for(int i = 0; i < 3; ++i) {
            auto v = co_await ch.recv();
            REQUIRE(v.has_value());
        }
    };

    sync_wait(when_all(sender_a(), sender_b(), consumer()));

    // FIFO：先挂起的 sender_a 先入槽。
    REQUIRE(produced.size() == 2);
    CHECK(produced[0] == 2);
    CHECK(produced[1] == 3);
    CHECK(ch.empty());
}

TEST_CASE("channel: 无缓冲同步交接（capacity 0）") {
    channel<int> ch{0};
    CHECK(ch.capacity() == 0);
    CHECK(ch.empty());
    CHECK(ch.full()); // 无缓冲无槽位，恒视为满

    std::optional<int> got;
    auto receiver = [&]() -> task<void> {
        auto v = co_await ch.recv();
        if(v.has_value()) {
            got = std::move(*v);
        } else {
            FAIL("unexpected closed");
        }
    };
    auto sender = [&]() -> task<void> {
        CHECK(co_await ch.send(42) == cr::send::kSent);
    };

    sync_wait(when_all(sender(), receiver()));

    REQUIRE(got.has_value());
    CHECK(*got == 42);
    CHECK(ch.empty());
}

TEST_CASE("channel: 无缓冲多接收者 FIFO 公平") {
    channel<int> ch{0};
    std::vector<int> order;
    std::vector<int> values;
    auto receiver = [&](int id) -> task<void> {
        auto v = co_await ch.recv();
        if(v.has_value()) {
            values.push_back(*v);
            order.push_back(id);
        }
    };
    auto sender = [&](int v) -> task<void> {
        CHECK(co_await ch.send(v) == cr::send::kSent);
    };

    sync_wait(when_all(receiver(1), receiver(2), sender(10), sender(20)));

    // FIFO：先挂起的 receiver(1) 先被喂。
    REQUIRE(order.size() == 2);
    CHECK(order[0] == 1);
    CHECK(order[1] == 2);
    REQUIRE(values.size() == 2);
    CHECK(values[0] == 10);
    CHECK(values[1] == 20);
}

TEST_CASE("channel: try_send / try_recv 非阻塞") {
    channel<int> ch{1};
    CHECK(ch.try_send(7) == cr::send::kSent);
    CHECK(ch.try_send(8) == cr::send::kFull);

    auto v = ch.try_recv();
    REQUIRE(v.has_value());
    CHECK(*v == 7);
    auto empty_result = ch.try_recv();
    REQUIRE(!empty_result.has_value());
    CHECK(empty_result.error() == cr::recv::kEmpty);

    channel<int> rendezvous{0};
    auto empty_rz = rendezvous.try_recv();
    REQUIRE(!empty_rz.has_value());
    CHECK(empty_rz.error() == cr::recv::kEmpty);
    CHECK(rendezvous.try_send(1) == cr::send::kFull);
}

TEST_CASE("channel: close 后 drain 剩余元素再 kClosed") {
    channel<int> ch{2};
    CHECK(sync_wait(ch.send(1)) == cr::send::kSent);
    CHECK(sync_wait(ch.send(2)) == cr::send::kSent);

    sync_wait(ch.close());
    CHECK(ch.closed());
    CHECK(ch.try_send(3) == cr::send::kClosed);

    auto a = sync_wait(ch.recv());
    REQUIRE(a.has_value());
    CHECK(*a == 1);
    auto b = sync_wait(ch.recv());
    REQUIRE(b.has_value());
    CHECK(*b == 2);
    auto c = sync_wait(ch.recv());
    REQUIRE(!c.has_value());
    CHECK(c.error() == cr::recv::kClosed);
}

TEST_CASE("channel: close 唤醒挂起的消费者与生产者") {
    channel<int> ch{0};
    std::optional<int> got;
    bool got_closed = false;
    auto receiver = [&]() -> task<void> {
        auto v = co_await ch.recv();
        if(v.has_value()) {
            got = std::move(*v);
        } else {
            got_closed = true;
        }
    };
    auto closer = [&]() -> task<void> { co_await ch.close(); };

    sync_wait(when_all(receiver(), closer()));
    CHECK(got_closed);
    CHECK(!got.has_value());

    // 挂起的生产者被唤醒返回 kClosed。
    channel<int> ch2{1};
    CHECK(sync_wait(ch2.send(1)) == cr::send::kSent);
    bool sender_closed = false;
    auto sender = [&]() -> task<void> {
        sender_closed = (co_await ch2.send(2) == cr::send::kClosed);
    };
    auto closer2 = [&]() -> task<void> { co_await ch2.close(); };
    sync_wait(when_all(sender(), closer2()));
    CHECK(sender_closed);
}
