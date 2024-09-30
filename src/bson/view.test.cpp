#include "./build.test.hpp"

#include <bson/types.h>
#include <bson/view.h>

#include <catch2/catch_test_macros.hpp>

using bson::testing::chars;

using std::operator""sv;

static_assert(std::forward_iterator<::bson_iterator>);
static_assert(std::ranges::forward_range<bson::view>);

// BSON regular expressions are the jankiest data type, so we need to test those properly
TEST_CASE("bson/view/regex/Normal") {
    // clang-format off
    bson_byte dat[] = {
        13, 0, 0, 0,
        bson_type_regex, 'r', 0,
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
    CHECK(it->type() == bson_type_regex);
    CHECK(it->regex().regex == "foo"sv);
    CHECK(it->regex().options == ""sv);
}

TEST_CASE("bson/view/regex/Misisng null") {
    // clang-format off
    bson_byte dat[] = {
        12, 0, 0, 0,
        bson_type_regex, 'r', 0,
        // rx
        'f', 'o', 'o', // There should be a null here
        // opts
        0,
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it.error() == bson_iter_errc_short_read);
}

TEST_CASE("bson/view/regex/Misisng both nulls") {
    // clang-format off
    bson_byte dat[] = {
        11, 0, 0, 0,
        bson_type_regex, 'r', 0,
        // rx
        'f', 'o', 'o', // There should be a null here
        // Missing options
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it.error() == bson_iter_errc_short_read);
}

TEST_CASE("bson/view/regex/Two empty strings") {
    // clang-format off
    bson_byte dat[] = {
        10, 0, 0, 0,
        bson_type_regex, 'r', 0,
        // rx
        '\0', // Empty regex
        '\0', // Empty options
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it->type() == bson_type_regex);
    CHECK(it->regex().regex == ""sv);
    CHECK(it->regex().options == ""sv);
    CHECK((++it) == v.end());
}

TEST_CASE("bson/view/regex/Missing strings") {
    // clang-format off
    bson_byte dat[] = {
        8, 0, 0, 0,
        bson_type_regex, 'r', 0, // No here
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it.error() == bson_iter_errc_short_read);
}

TEST_CASE("bson/view/regex/Extra null") {
    // clang-format off
    bson_byte dat[] = {
        17, 0, 0, 0,
        bson_type_regex, 'r', 0,
        'f', 'o', 'o', '\0', '\0',
        'b', 'a', 'r', '\0',
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it->type() == bson_type_regex);
    CHECK(it->regex().regex == "foo"sv);
    // The second null caused our options to truncate
    CHECK(it->regex().options == ""sv);
    ++it;
    CHECK(it.error() == bson_iter_errc_invalid_type);
}
