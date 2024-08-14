#include "./just.hpp"
#include "./concepts.hpp"

#include <catch2/catch.hpp>

using namespace amongoc;

static_assert(nanosender<just<int>>);
static_assert(multishot_nanosender<just<int>>);
static_assert(multishot_nanosender<just<int&>>);
static_assert(multishot_nanosender<just<const int&>>);
static_assert(nanosender<just<std::unique_ptr<int>>>);
// A just() of non-copyable is not multi-shot, since it must move its argument
static_assert(not multishot_nanosender<just<std::unique_ptr<int>>>);
// A reference-to-noncopyable is copyable itself, though:
static_assert(multishot_nanosender<just<std::unique_ptr<int>&>>);

TEST_CASE("just/42") {
    auto               x = amongoc::just(42);
    std::optional<int> oi;
    auto               op = x.connect([&](auto v) { oi = v; });
    op.start();
    CHECK(oi == 42);
}

TEST_CASE("just/Multi-shot") {
    auto                       x = amongoc::just(std::string("hey"));
    std::optional<std::string> got;
    {
        auto op = x.connect([&](auto&& x) { got.emplace(NEO_FWD(x)); });
        op.start();
    }
    CHECK(got == "hey");
    {
        auto op = x.connect([&](auto&& x) { got.emplace(NEO_FWD(x)); });
        op.start();
    }
    CHECK(got == "hey");
}
