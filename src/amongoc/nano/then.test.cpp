#include "./then.hpp"
#include "./just.hpp"

#include <catch2/catch.hpp>

using namespace amongoc;

static_assert(nanosender<then_t<just<int>, std::function<double(int)>>>);
static_assert(multishot_nanosender<then_t<just<int>, std::function<double(int)>>>);

TEST_CASE("then/operator|") {
    nanosender_of<std::string> auto c  //
        = just(42)                     //
        | then([](int n) { return std::to_string(n); });

    bool did_run = false;
    auto op      = c.connect([&](std::string s) {
        CHECK(s == "42");
        did_run = true;
    });
    op.start();
    CHECK(did_run);
}
