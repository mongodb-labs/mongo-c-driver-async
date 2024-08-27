#include "amongoc/alloc.h"
#include "amongoc/box.h"
#include "amongoc/emitter.h"
#include "amongoc/loop.h"
#include "amongoc/operation.h"
#include "amongoc/status.h"
#include <amongoc/async.h>

#include "./coroutine.hpp"
#include <amongoc/default_loop.h>

#include <catch2/catch.hpp>

using namespace amongoc;

emitter returns_42() {
    return amongoc_just(amongoc_okay, amongoc_box_int(42), amongoc_default_allocator);
}

TEST_CASE("Async/Just") {
    auto        em = returns_42().as_unique();
    amongoc_box got;
    auto        op = amongoc_tie(AM_FWD(em).release(), nullptr, &got).as_unique();
    op.start();
    CHECK(got.view.as<int>() == 42);
}

TEST_CASE("Async/Transform with the C API") {
    auto em = returns_42().as_unique();
    em      = amongoc_then(AM_FWD(em).release(),
                      amongoc_async_default,
                      amongoc_default_allocator,
                      amongoc_nil,
                      [](box, status* st, box value) noexcept {
                          amongoc_box_cast(int)(value) = 81 + amongoc_box_cast(int)(value);
                          return value;
                      })
             .as_unique();
    unique_box       got{amongoc_nil};
    unique_operation op
        = std::move(em).connect(terminating_allocator,
                                [&](status st, unique_box b) { got = std::move(b); });
    op.start();
    CHECK(got.as<int>() == 123);
}

TEST_CASE("Async/Timeout") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    // One minute delay (too slow)
    emitter big_delay = amongoc_schedule_later(&loop, timespec{30});
    // Half second timeout:
    auto   timed = amongoc_timeout(&loop, big_delay, timespec{0, 500'000'000}).as_unique();
    status got;
    auto   op
        = std::move(timed).connect(terminating_allocator, [&](status st, unique_box) { got = st; });
    op.start();
    auto start = std::chrono::steady_clock::now();
    amongoc_default_loop_run(&loop);
    auto end = std::chrono::steady_clock::now();
    CHECK(amongoc_is_timeout(got));
    // Should have stopped very quickly
    CHECK((end - start) < std::chrono::seconds(3));
    amongoc_default_loop_destroy(&loop);
}

TEST_CASE("Async/then_just") {
    auto em
        = amongoc_just(amongoc_okay, amongoc_box_int(42), amongoc_default_allocator).as_unique();
    em = amongoc_then_just(AM_FWD(em).release(),
                           amongoc_async_forward_errors,
                           amongoc_okay,
                           amongoc_box_int(1729),
                           amongoc_default_allocator)
             .as_unique();
    status st;
    box    val;
    auto   op = amongoc_tie(AM_FWD(em).release(), &st, &val).as_unique();
    op.start();
    CHECK(st.code == 0);
    CHECK(st.category == &amongoc_generic_category);
    CHECK(val.view.as<int>() == 1729);
}

// Write an emitter type that forcibly allocates, to test that let() destroys the objects
// at the appropriate time
emitter waits(amongoc_loop& loop) {
    co_await amongoc_schedule_later(&loop, timespec{0, 1000 * 5}).as_unique();
    co_return amongoc_nil.as_unique();
}

TEST_CASE("Async/let") {
    amongoc_loop loop;
    amongoc_default_loop_init(&loop);
    auto em = amongoc_let(waits(loop),
                          amongoc_async_forward_errors,
                          amongoc_default_allocator,
                          amongoc_box_pointer(&loop),
                          [](amongoc_box loop_ptr, amongoc_status, amongoc_box) noexcept {
                              return waits(*loop_ptr.view.as<amongoc_loop*>());
                          })
                  .as_unique();
    amongoc_box fin = amongoc_nil;
    auto        op  = amongoc_tie(AM_FWD(em).release(), nullptr, &fin);
    amongoc_start(&op);
    amongoc_default_loop_run(&loop);
    amongoc_operation_destroy(op);
    amongoc_default_loop_destroy(&loop);
}
