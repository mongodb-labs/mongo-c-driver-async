#include "./event_emitter.hpp"

#include <amongoc/nano/util.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace amongoc;

TEST_CASE("event_emitter/Simple listener") {
    event_emitter<int> int_event;
    int                got = 0;
    event_listener     l{int_event, [&](int a) { got = a; }};
    int_event.fire(42);
    CHECK(got == 42);
}

TEST_CASE("event_emitter/Ordered listeners") {
    int                first_fired  = 0;
    int                second_fired = 0;
    event_emitter<int> ev;
    auto               l1 = ev.listen([&](int a) {
        // The first listener will dispatch after the second was fired
        CHECK(second_fired == a);
        first_fired = a;
    });
    auto               l2 = ev.listen([&](int a) {
        // The second listener will dispatch first
        CHECK(first_fired == 0);
        second_fired = a;
    });
    ev.fire(42);
}

TEST_CASE("event_emitter/Destruction") {
    int                got = 0;
    event_emitter<int> ev;
    {
        auto l1 = ev.listen(assign(got));
        {
            auto l2 = ev.listen([&](int a) { FAIL_CHECK("This should never fire"); });
        }
        ev.fire(42);
    }
    CHECK(got == 42);
}
