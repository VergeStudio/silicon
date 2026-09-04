





#include <string>

#include <silicon/test/test.h>

import silicon.event;

using namespace silicon::event;

TEST_CASE("event_status 枚举底层值（kSuccess/kFailure/kTimeout）") {
    static_assert(static_cast<int>(event_status::kSuccess) == 0);
    static_assert(static_cast<int>(event_status::kFailure) == 1);
    static_assert(static_cast<int>(event_status::kTimeout) == 2);
}

TEST_CASE("event 缺省构造：空名、状态 kSuccess") {
    event e;
    CHECK(e.name().empty());
    CHECK(e.status() == event_status::kSuccess);
}

TEST_CASE("event 命名构造：name/status 正确") {
    event e{"render_frame"};
    CHECK(e.name() == "render_frame");
    CHECK(e.status() == event_status::kSuccess);
}

TEST_CASE("event set_status 回写状态") {
    event e{"tick"};
    e.set_status(event_status::kFailure);
    CHECK(e.status() == event_status::kFailure);
    e.set_status(event_status::kTimeout);
    CHECK(e.status() == event_status::kTimeout);
}

TEST_CASE("event error category 自注册：非注入消费方也能构造 error_code（不再 terminate）") {
    auto ec = make_error_code(event_error::kInvalidStatus);
    CHECK(ec);
    CHECK(ec.value() == static_cast<int>(event_error::kInvalidStatus));
    CHECK(std::string(ec.category().name()) == "silicon.event");
    CHECK(ec.message() == "invalid event status");
}
