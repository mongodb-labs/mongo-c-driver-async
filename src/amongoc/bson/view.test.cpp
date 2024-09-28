#include "./build.test.hpp"

#include <amongoc/bson/types.h>
#include <amongoc/bson/view.h>

#include <catch2/catch_test_macros.hpp>

using bson::testing::chars;

using std::operator""sv;

// BSON regular expressions are the jankiest data type, so we need to test those properly
TEST_CASE("bson/view/regex/Normal") {
    // clang-format off
    bson_byte dat[] = {
        13, 0, 0, 0,
        BSON_TYPE_REGEX, 'r', 0,
        // rx
        'f', 'o', 'o', 0,
        // opts
        0,
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it->key() == "r");
    CHECK(it->type() == BSON_TYPE_REGEX);
    CHECK(it->regex().regex == "foo"sv);
    CHECK(it->regex().options == ""sv);
}

TEST_CASE("bson/view/regex/Misisng null") {
    // clang-format off
    bson_byte dat[] = {
        12, 0, 0, 0,
        BSON_TYPE_REGEX, 'r', 0,
        // rx
        'f', 'o', 'o', // There should be a null here
        // opts
        0,
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it.error() == BSON_ITER_SHORT_READ);
}

TEST_CASE("bson/view/regex/Misisng both nulls") {
    // clang-format off
    bson_byte dat[] = {
        11, 0, 0, 0,
        BSON_TYPE_REGEX, 'r', 0,
        // rx
        'f', 'o', 'o', // There should be a null here
        // Missing options
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it.error() == BSON_ITER_SHORT_READ);
}

TEST_CASE("bson/view/regex/Two empty strings") {
    // clang-format off
    bson_byte dat[] = {
        10, 0, 0, 0,
        BSON_TYPE_REGEX, 'r', 0,
        // rx
        '\0', // Empty regex
        '\0', // Empty options
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it->type() == BSON_TYPE_REGEX);
    CHECK(it->regex().regex == ""sv);
    CHECK(it->regex().options == ""sv);
    CHECK((++it) == v.end());
}

TEST_CASE("bson/view/regex/Missing strings") {
    // clang-format off
    bson_byte dat[] = {
        8, 0, 0, 0,
        BSON_TYPE_REGEX, 'r', 0, // No here
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it.error() == BSON_ITER_SHORT_READ);
}

TEST_CASE("bson/view/regex/Extra null") {
    // clang-format off
    bson_byte dat[] = {
        17, 0, 0, 0,
        BSON_TYPE_REGEX, 'r', 0,
        'f', 'o', 'o', '\0', '\0',
        'b', 'a', 'r', '\0',
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it->type() == BSON_TYPE_REGEX);
    CHECK(it->regex().regex == "foo"sv);
    // The second null caused our options to truncate
    CHECK(it->regex().options == ""sv);
    ++it;
    CHECK(it.error() == BSON_ITER_INVALID_TYPE);
}
