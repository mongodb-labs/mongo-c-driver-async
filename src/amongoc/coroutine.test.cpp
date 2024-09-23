#include "./coroutine.hpp"

#include "./nano/just.hpp"
#include "./nano/tie.hpp"

#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/default_loop.h>
#include <amongoc/emitter.h>
#include <amongoc/loop.h>
#include <amongoc/operation.h>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <memory>

using namespace amongoc;

static_assert(nanosender<co_task<int>::sender>);

emitter basic_coro(mlib_allocator = mlib_default_allocator) { co_return 0; }

TEST_CASE("Coroutine/Basic") {
    auto em      = basic_coro().as_unique();
    bool did_run = false;
    auto op      = std::move(em).bind_allocator_connect(mlib::terminating_allocator,
                                                   [&](emitter_result&&) { did_run = true; });
    op.start();
    CHECK(did_run);
}

TEST_CASE("Coroutine/Discard") {
    auto em = basic_coro().as_unique();
    // Discard a the coroutine without starting it. (Shouldn't leak memory)
}

TEST_CASE("Coroutine/Discard After Connect") {
    auto op = basic_coro().as_unique().bind_allocator_connect(mlib::terminating_allocator,
                                                              [](auto...) {});
    // Discard the connected operation. Should not leak memory
}

co_task<int> cxx_coro(allocator<> = allocator<>{mlib_default_allocator}) { co_return 42; }

TEST_CASE("Coroutine/co_task") {
    auto co  = cxx_coro();
    int  got = 1729;
    auto op  = std::move(co).as_sender().connect([&](auto x) { got = *x; });
    CHECK(got == 1729);
    op.start();
    CHECK(got == 42);
}

TEST_CASE("Coroutine/Discard co_task") {
    // Do nothing with the coroutine. Should not leak memory.
    { auto co = cxx_coro(); }
}

TEST_CASE("Coroutine/Discard co_task operation") {
    // Do nothing with the coroutine. Should not leak memory.
    auto co  = cxx_coro();
    int  got = 0;
    {
        std::move(co).as_sender().connect([&](auto x) { got = *x; });
    }
    // Coroutine did not execute
    CHECK(got == 0);
}

co_task<std::unique_ptr<int>> returns_unique(allocator<>) { co_return std::make_unique<int>(42); }

co_task<std::unique_ptr<int>> nested_returns_unique(allocator<> a) {
    auto co   = returns_unique(a);
    auto iptr = co_await std::move(co);
    *iptr += 81;
    co_return iptr;
}

TEST_CASE("Coroutine/co_task nested awaiting") {
    co_task<std::unique_ptr<int>>::result_type iptr;
    auto op = nested_returns_unique(allocator<>{mlib_default_allocator}).as_sender() | tie(iptr);
    op.start();
    CHECK(**iptr == 123);
}

struct my_error {
    int value;
};

co_task<int> throws_an_error(allocator<>) {
    throw my_error{1729};
    co_return 42;
}

co_task<int> catches_an_error(allocator<> a) {
    try {
        co_return co_await throws_an_error(a);
    } catch (my_error e) {
        co_return e.value;
    }
}

TEST_CASE("Coroutine/Exception propagation") {
    co_task<int>::result_type got = 0;
    auto op = catches_an_error(allocator<>{mlib_default_allocator}).as_sender() | tie(got);
    CHECK(*got == 0);
    op.start();
    CHECK(*got == 1729);
}

co_task<int> immediate_awaits(allocator<>) { co_return co_await just(42); }

TEST_CASE("Coroutine/immediate await") {
    co_task<int>::result_type got = 0;
    auto op = immediate_awaits(allocator<>{mlib_default_allocator}).as_sender() | tie(got);
    CHECK(*got == 0);
    op.start();
    CHECK(*got == 42);
}
