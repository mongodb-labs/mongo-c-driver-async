#include "./coroutine.hpp"

#include <amongoc/async.h>
#include <amongoc/box.h>
#include <amongoc/default_loop.h>
#include <amongoc/emitter.h>
#include <amongoc/loop.h>
#include <amongoc/operation.h>

#include <catch2/catch.hpp>
#include <chrono>

using namespace amongoc;

emitter basic_coro() { co_return 0; }

TEST_CASE("Coroutine/Basic") {
    auto em      = basic_coro().as_unique();
    bool did_run = false;
    auto op      = std::move(em).connect([&](status st, unique_box) { did_run = true; });
    op.start();
    CHECK(did_run);
}

TEST_CASE("Coroutine/Discard") {
    auto em = basic_coro().as_unique();
    // Discard a the coroutine without starting it. (Shouldn't leak memory)
}

TEST_CASE("Coroutine/Discard After Connect") {
    auto op = basic_coro().as_unique().connect([](auto...) {});
    // Discard the connected operation. Should not leak memory
}

co_task<int> cxx_coro() { co_return 42; }

TEST_CASE("Coroutine/co_task") {
    auto co  = cxx_coro();
    int  got = 1729;
    auto op  = std::move(co).connect([&](int x) { got = x; });
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
        std::move(co).connect([&](int x) { got = x; });
    }
    // Coroutine did not execute
    CHECK(got == 0);
}
