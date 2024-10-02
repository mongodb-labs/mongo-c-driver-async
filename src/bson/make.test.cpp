#include "./build.test.hpp"

#include <bson/make.hpp>
#include <bson/types.h>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

using namespace bson::testing;

TEST_CASE("bson/make/empty") {
    auto d = bson::make::doc().build(::mlib_default_allocator);
    CHECK_BSON_BYTES_EQ(d, chars{5, 0, 0, 0, 0});
}

TEST_CASE("bson/make/simple-string") {
    auto d = bson::make::doc(std::pair("foo", "bar")).build(::mlib_default_allocator);
    // clang-format off
    CHECK_BSON_BYTES_EQ(d, chars{
        18, 0, 0, 0,
        bson_type_utf8, 'f', 'o', 'o', '\0',
        4, 0, 0, 0, 'b', 'a', 'r', '\0',
        0,
    });
    // clang-format on
}

TEST_CASE("bson/make/optional-pair") {
    auto d = bson::make::doc(bson::make::conditional(std::make_optional(std::pair("foo", "bar"))))
                 .build(::mlib_default_allocator);
    // clang-format off
    CHECK_BSON_BYTES_EQ(d, chars{
        18, 0, 0, 0,
        bson_type_utf8, 'f', 'o', 'o', '\0',
        4, 0, 0, 0, 'b', 'a', 'r', '\0',
        0,
    });
    // clang-format on
    d = bson::make::doc(
            bson::make::conditional(std::optional<std::pair<std::string, std::string>>{}))
            .build(::mlib_default_allocator);
    // Nothing is appended:
    CHECK_BSON_BYTES_EQ(d, chars{5, 0, 0, 0, 0});
}
