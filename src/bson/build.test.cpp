#include <bson/build.h>
#include <bson/types.h>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

using bson::document;

TEST_CASE("bson/build/empty") {
    document doc{mlib_default_allocator};
    CHECK(doc.empty());
    CHECK(std::ranges::distance(doc) == 0);
}

TEST_CASE("bson/allocator propagation") {
    mlib_allocator_impl dup_impl = *mlib_default_allocator.impl;
    mlib_allocator      dup      = {&dup_impl};
    // A vector with our custom allocator should copy the same allocator into
    // child elements as they are inserted
    std::vector<document, mlib::allocator<document>> vec{mlib::allocator<>(dup)};
    vec.emplace_back();
    CHECK(vec.back().get_allocator() == vec.get_allocator());
}

TEST_CASE("bson/build/insert") {
    document doc{mlib_default_allocator};
    doc.emplace_back("foo", "bar");
    auto it = doc.begin();
    REQUIRE(it != doc.end());
    CHECK(it->key() == "foo");
    CHECK(std::string_view(it->utf8()) == "bar");
}

TEST_CASE("bson/build/subdoc") {
    document doc{::mlib_default_allocator};
    auto     child = doc.push_subdoc("foo");
    child.emplace_back("bar", "baz");
    auto it = doc.begin();
    REQUIRE(it->type() == BSON_TYPE_DOCUMENT);
    auto s     = it->document();
    auto subit = s.begin();
    CHECK(subit->key() == "bar");
    CHECK(subit->utf8() == "baz");
}
