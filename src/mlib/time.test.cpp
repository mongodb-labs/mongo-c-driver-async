#include <mlib/time.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

using namespace std::literals;

TEST_CASE("mlib/time/Saturating time") {
    auto dur = mlib_seconds(INT64_MAX);  // Too large to actually represent
    CHECK(::mlib_microseconds_count(dur) == INT64_MAX);
    CHECK(::mlib_seconds_count(dur) != INT64_MAX);
}
