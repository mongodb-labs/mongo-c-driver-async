#include "./algorithm.hpp"

#include <catch2/catch_test_macros.hpp>

#include <ranges>

TEST_CASE("mlib/algorithm/copy_to_capacity") {
    std::vector<int> vec;
    SECTION("Zero capacity") {
        mlib::copy_to_capacity(std::views::iota(0, 500), vec);
        // Nothing was copyied, because the vector starts with zero capacity
        CHECK(vec.size() == 0);
    }

    SECTION("Stops at capacity") {
        vec.reserve(10);
        mlib::copy_to_capacity(std::views::iota(0, 500), vec);
        // Only ten items were inserted, because that is the vector's capacity
        CHECK(vec.size() == 10);
    }

    SECTION("Stops when input range is done") {
        vec.reserve(500);
        mlib::copy_to_capacity(std::views::iota(0, 15), vec);
        // Stopped when the input range is finished.
        CHECK(vec.size() == 15);
        CHECK(vec.capacity() >= 500);
    }

    SECTION("Inserts in the correct order") {
        vec.reserve(5);
        mlib::copy_to_capacity(std::views::iota(0, 10), vec);
        REQUIRE(vec.size() >= 5);
        CHECK(vec[0] == 0);
        CHECK(vec[1] == 1);
        CHECK(vec[2] == 2);
        CHECK(vec[3] == 3);
        CHECK(vec[4] == 4);
    }
}
