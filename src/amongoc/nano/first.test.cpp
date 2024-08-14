#include "./first.hpp"
#include "amongoc/nano/branch.hpp"
#include "amongoc/nano/just.hpp"

#include <catch2/catch.hpp>

using namespace amongoc;

TEST_CASE("first_completed/Simple 1") {
    auto two   = first_completed(just(41), just(42));
    int  got   = 0;
    int  calls = 0;
    auto op    = std::move(two).connect([&](auto x) -> void {
        std::visit(
            [&](auto x) {
                got = x;
                ++calls;
            },
            x);
    });
    op.start();
    CHECK(calls == 1);
    CHECK((got == 41 or got == 42));
}

TEST_CASE("first_completed/With variant branch") {
    auto two   = first_completed(just(41), just(42));
    int  got   = 0;
    int  calls = 0;
    auto op    = std::move(two).connect(amongoc::branch(
        [&](auto n) {
            got = n;
            ++calls;
        },
        [&](auto y) {
            got = y;
            ++calls;
        }));
    op.start();
    CHECK(calls == 1);
    CHECK((got == 41 or got == 42));
}

TEST_CASE("first_completed/No operands") {
    auto none = first_completed();
    auto op   = std::move(none).connect([](auto&& x) {});
    if (0) {
        // Will not be called, but this verifies that it can compile.
        // (Executing this would terminate)
        op.start();
    }
}
