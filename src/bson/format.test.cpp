#include <bson/doc.h>
#include <bson/format.h>
#include <bson/make.hpp>
#include <bson/mut.h>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("bson/format empty") {
    bson::document empty{::mlib_default_allocator};
    mlib_str       s = mlib_str_new().str;
    ::bson_write_repr(&s, empty);
    CHECK(s == "{ }");
    mlib_str_delete(s);
}

TEST_CASE("bson/format to std::string") {
    bson::document empty{::mlib_default_allocator};
    std::string    s;
    ::bson_write_repr(s, empty);
    CHECK(s == "{ }");
}

TEST_CASE("bson/format non-empty") {
    auto        doc = bson::make::doc(bson::make::pair("hey", 42)).build(::mlib_default_allocator);
    std::string s;
    ::bson_write_repr(s, doc);
    CHECK(s == R"({ "hey": 42:i32 })");
    bson::mutator(doc).emplace_back("foo", "bar");
    s.clear();
    ::bson_write_repr(s, doc);
    CHECK(s == R"({
  "hey": 42:i32,
  "foo": "bar",
})");
    ;
}

TEST_CASE("bson/format nested") {
    using namespace bson::make;
    auto doct = doc(pair("doc", doc(pair("foo", "bar"), pair("baz", "quux"))))
                    .build(::mlib_default_allocator);
    std::string s;
    ::bson_write_repr(s, doct);
    CHECK(s == R"({
  "doc": {
    "foo": "bar",
    "baz": "quux",
  },
})");
}

TEST_CASE("bson/format oneline") {
    using namespace bson::make;
    auto doct = doc(pair("doc", doc(pair("foo", "bar"), pair("baz", "quux"))))
                    .build(::mlib_default_allocator);
    std::string      s;
    bson_fmt_options opts{};
    ::bson_write_repr(s, doct, &opts);
    CHECK(s == R"({ "doc": { "foo": "bar", "baz": "quux" } })");
}
