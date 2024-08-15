#include "amongoc/box.h"
#include "amongoc/emitter.h"
#include <amongoc/async.h>

#include <amongoc/default_loop.h>

#include <catch2/catch.hpp>

using namespace amongoc;

emitter returns_42() { return amongoc_just(amongoc_okay, unique_box::from(42).release()); }

TEST_CASE("Async/Just") {
    auto        em = returns_42().as_unique();
    amongoc_box got;
    auto        op = amongoc_tie(em.release(), nullptr, &got).as_unique();
    op.start();
    CHECK(got.view.as<int>() == 42);
}

TEST_CASE("Async/Transform with the C API") {
    auto em = returns_42().as_unique();
    em      = amongoc_then(em.release(),
                      amongoc_then_default,
                      amongoc_nothing,
                      [](box, status* st, box value) noexcept {
                          amongoc_box_cast(int)(value) = 81 + amongoc_box_cast(int)(value);
                          return value;
                      })
             .as_unique();
    unique_box       got{amongoc_nothing};
    unique_operation op
        = std::move(em).connect([&](status st, unique_box b) { got = std::move(b); });
    op.start();
    CHECK(got.as<int>() == 123);
}

TEST_CASE("Async/Timeout") {
    amongoc_loop loop;
    amongoc_init_default_loop(&loop);
    // One minute delay (too slow)
    emitter big_delay = amongoc_schedule_later(&loop, 5 * 1000 * 1000);
    // Half second timeout:
    auto   timed = amongoc_timeout_us(&loop, big_delay, 500 * 1000).as_unique();
    status got;
    auto   op = std::move(timed).connect([&](status st, unique_box) { got = st; });
    op.start();
    auto start = std::chrono::steady_clock::now();
    amongoc_run_default_loop(&loop);
    auto end = std::chrono::steady_clock::now();
    CHECK(amongoc_is_timeout(got));
    // Should have stopped very quickly
    CHECK((end - start) < std::chrono::seconds(3));
    amongoc_destroy_default_loop(&loop);
}
