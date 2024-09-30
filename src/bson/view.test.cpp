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

/**
 * @brief Test that the `break` keyword in a `bson_foreach` loop will stop the
 * loop properly.
 */
TEST_CASE("bson/view/foreach/break") {
    // clang-format off
    bson_byte dat[] = {
        38, 0, 0, 0,
        // String
        bson_type_utf8, 'r', 0,
        4, 0, 0, 0, 'f', 'o', 'o', 0,
        // int64
        bson_type_int64, 'b', 0,
        42, 0, 0, 0, 0, 0, 0, 0,
        // Subdocument
        bson_type_document, 'c', 0,
        8, 0, 0, 0, 'l', 'o', 'l', 0,
        0,
    };
    // clang-format on
    auto          v = bson::view::from_data(dat, sizeof dat);
    bson_iterator last_seen;
    int           nth = 0;
    // There are three elements, but we stop at two
    bson_foreach(iter, v) {
        last_seen = iter;
        ++nth;
        if (nth == 2) {
            break;
        }
    }
    // We broke out on the second iteration
    CHECK(nth == 2);
    CHECK(last_seen->key() == "b");
}

/**
 * @brief Test that `bson_foreach` over a document will continue up-to and including
 * an iterator pointing to an errant element. It should stop on that element
 * and not try to continue
 */
TEST_CASE("bson/view/foreach/error iterator") {
    // clang-format off
    bson_byte dat[] = {
        38, 0, 0, 0,
        // String
        bson_type_utf8, 'r', 0,
        4, 0, 0, 0, 'f', 'o', 'o', 0,
        // int64
        bson_type_int64, 'b', 0,
        42, 0, 0, 0, 0, 0, 0, 0,
        // Malformed
        bson_type_document, 'c', 0,
        64, 0, 0, 0, 'l', 'o', 'l', 0,
        0,
    };
    // clang-format on
    auto v  = bson::view::from_data(dat, sizeof dat);
    auto it = v.begin();
    CHECK(it->key() == "r");
    CHECK(it->utf8() == "foo");
    ++it;
    CHECK(it->key() == "b");
    CHECK(it->int64() == 42);
    ++it;
    CHECK(it.has_error());

    int  nth       = 0;
    bool got_error = false;
    bson_foreach(iter, v) {
        switch (nth++) {
        case 0:
            CHECK(iter->key() == "r");
            continue;
        case 1:
            CHECK(iter->key() == "b");
            continue;
        case 2:
            CHECK(iter.error() == bson_iter_errc::bson_iter_errc_invalid_length);
            got_error = true;
            continue;
        case 3:
            FAIL_CHECK("Should have stopped");
        }
    }
    CHECK(got_error);
    CHECK(nth == 3);
}
