#include <bson/doc.h>
#include <bson/mut.h>
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

/**
 * @brief Ensure copying an empty document works.
 *
 * This will hit the odd code path where it will need to copy data from the
 * global empty instance. We don't want it to attempt to write any data in that
 * case.
 */
TEST_CASE("bson/doc/copy empty") {
    ::bson_doc empty = bson_new();
    ::bson_doc dup   = bson_new(empty);
    (void)dup;
    // Do not delete the objects. They are guaranteed to not allocate, so there
    // is no memory to leak. If LeakSanitizer fails this test, that's a bug.
}

TEST_CASE("bson/doc/copy generic") {
    auto doc  = ::bson_new();
    auto view = ::bson_as_view(doc);
    auto mut  = ::bson_mutate(&doc);
    ::bson_new();
    ::bson_new(5);
    ::bson_new(5, mlib_default_allocator);
    ::bson_new(mlib_default_allocator);
    ::bson_new(doc);
    ::bson_new(view);
    ::bson_new(mut);
    ::bson_new(view, mlib_default_allocator);
    ::bson_new(doc, mlib_default_allocator);
    ::bson_new(mut, mlib_default_allocator);
    // All of the above APIs are guaranteed to not allocate because the docs are
    // empty.
}

TEST_CASE("bson/build/insert") {
    document doc{mlib_default_allocator};
    bson::mutator(doc).emplace_back("foo", "bar");
    auto it = doc.begin();
    REQUIRE(it != doc.end());
    CHECK(it->key() == "foo");
    CHECK(std::string_view(it->value().utf8) == "bar");
}

TEST_CASE("bson/build/subdoc") {
    document doc{::mlib_default_allocator};
    auto     m1    = bson::mutator(doc);
    auto     child = m1.push_subdoc("foo");
    child.emplace_back("bar", "baz");
    auto it = doc.begin();
    REQUIRE(it->type() == bson_type_document);
    auto s     = it->value().document;
    auto subit = s.begin();
    CHECK(subit->key() == "bar");
    CHECK(subit->value().utf8 == "baz");
    it = child.position_in_parent();
    CHECK(it->type() == bson_type_document);
    CHECK(it->key() == "foo");
}
