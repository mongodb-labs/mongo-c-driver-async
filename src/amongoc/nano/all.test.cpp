#include "./all.hpp"

#include "./just.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace amongoc;

TEST_CASE("when_all/Single Operation") {
    auto            one = when_all(just(31));
    std::tuple<int> got;
    auto            op = std::move(one).connect([&](auto r) { got = r; });
    op.start();
    CHECK(std::get<0>(got) == 31);
}

TEST_CASE("when_all/Start Multiple Operations") {
    auto                 multi = when_all(just(31), just(42));
    std::tuple<int, int> got;
    auto                 op = std::move(multi).connect([&](auto r) { got = r; });
    op.start();
    auto [x, y] = got;
    CHECK(x == 31);
    CHECK(y == 42);
}

TEST_CASE("when_all/No Operations") {
    nanosender_of<std::tuple<>> auto none    = when_all();
    bool                             did_run = false;
    auto op = std::move(none).connect([&](std::tuple<>) { did_run = true; });
    CHECK_FALSE(did_run);
    op.start();
    CHECK(did_run);
}
