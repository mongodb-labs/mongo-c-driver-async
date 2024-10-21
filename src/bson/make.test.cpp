#include "./build.test.hpp"

#include <bson/make.hpp>
#include <bson/types.h>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

using namespace bson::testing;

using namespace bson::make;

TEST_CASE("bson/make/empty") {
    auto d = doc().build(::mlib_default_allocator);
    CHECK_BSON_BYTES_EQ(d, chars{5, 0, 0, 0, 0});
}

TEST_CASE("bson/make/simple-string") {
    auto d = doc(pair("foo", "bar")).build(::mlib_default_allocator);
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
    auto d
        = doc(conditional(std::make_optional(pair("foo", "bar")))).build(::mlib_default_allocator);
    // clang-format off
    CHECK_BSON_BYTES_EQ(d, chars{
        18, 0, 0, 0,
        bson_type_utf8, 'f', 'o', 'o', '\0',
        4, 0, 0, 0, 'b', 'a', 'r', '\0',
        0,
    });
    // clang-format on
    d = doc(conditional(std::optional<pair<std::string>>{})).build(::mlib_default_allocator);
    // Nothing is appended:
    CHECK_BSON_BYTES_EQ(d, chars{5, 0, 0, 0, 0});
}

TEST_CASE("bson/make/nested-doc") {
    auto d = doc(pair("foo", doc())).build(::mlib_default_allocator);
    CHECK_BSON_BYTES_EQ(d, chars{15, 0, 0, 0, 3, 'f', 'o', 'o', 0, 5, 0, 0, 0, 0, 0});
}
