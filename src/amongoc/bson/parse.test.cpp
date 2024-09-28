#include <amongoc/bson/build.h>
#include <amongoc/bson/make.hpp>
#include <amongoc/bson/parse.hpp>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

using namespace bson;
using namespace bson::make;
using namespace bson::parse;
using std::pair;

static document buildit(auto spec) { return spec.build(::mlib_default_allocator); }

TEST_CASE("bson/parse/simple") {
    auto             d = buildit(make::doc(pair("foo", "bar")));
    std::string_view got;
    auto             r = any(field("foo", parse::type<std::string_view>(store(got))))(d);
    CHECK(did_accept(r));
    auto r2 = parse::any(must(field("baz", type<std::string_view>())),
                         field("quux", just_accept{}),
                         field("foo", just_accept{}))(d);
    CHECK_FALSE(did_accept(r2));
    CHECK(describe_error(r2) == "errors: [element ‘baz’ not found]");
}

TEST_CASE("bson/parse/doc") {
    // Field "bar" is optional, field "foo" is required
    rule<bson::view> auto d = parse::doc(field("bar", just_accept{}),
                                         must(field("foo", type<std::int32_t>())),
                                         just_accept{});
    auto                  r = d(buildit(make::doc(pair("foo", 21))));
    CAPTURE(describe_error(r));
    CHECK(did_accept(r));
}

TEST_CASE("bson/parse/doc missing one required") {
    // "bar" and "foo" are both required
    rule<bson::view> auto d = parse::doc(must(field("bar", just_accept{})),
                                         must(field("foo", type<std::int32_t>())),
                                         just_accept{});
    auto                  r = d(buildit(make::doc(pair("bar", 21))));
    CAPTURE(describe_error(r));
    CHECK(describe_error(r) == "errors: [missing required element ‘foo’]");
    CHECK_FALSE(did_accept(r));
}

TEST_CASE("bson/parse/doc missing two required") {
    rule<bson::view> auto d = parse::doc(field("bar", just_accept{}),
                                         must(field("foo", type<std::int32_t>())),
                                         must(field("baz", type<std::int32_t>())),
                                         just_accept{});
    auto                  r = d(buildit(make::doc(pair("bar", 21))));
    CAPTURE(describe_error(r));
    CHECK(describe_error(r)
          == "errors: [missing required element ‘foo’, missing required element ‘baz’]");
    CHECK_FALSE(did_accept(r));
}

TEST_CASE("bson/parse/doc optional field rejects, but accepts full doc") {
    rule<bson::view> auto d = parse::doc(field("bar", just_accept{}),
                                         field("foo", type<std::int32_t>()),
                                         just_accept{});
    auto                  r = d(buildit(make::doc(pair("foo", "string"))));
    CHECK(did_accept(r));
    CHECK(describe_error(r) == "[accepted]");
}

TEST_CASE("bson/parse/doc rejected optional field does not contribute error") {
    rule<bson::view> auto d = parse::doc(field("foo", type<std::int32_t>()),
                                         must(field("bar", just_accept{})),
                                         just_accept{});
    // The "foo" element will not be accepted because it has the wrong type, but
    // it should not contribute an error message because it is marked as optional
    auto r = d(buildit(make::doc(pair("foo", "string"))));
    CHECK_FALSE(did_accept(r));
    CHECK(describe_error(r) == "errors: [missing required element ‘bar’]");
}

TEST_CASE("bson/parse/doc rejects with optional field that generates a hard error") {
    rule<bson::view> auto d = parse::doc(field("foo", must(type<std::int32_t>())), just_accept{});
    // The "foo" element is optional, but if it appears then it must be an integer
    auto r = d(buildit(make::doc(pair("foo", "string"))));
    CHECK_FALSE(did_accept(r));
    CHECK(describe_error(r) == "errors: [in field ‘foo’: element has incorrect type]");
}

TEST_CASE("bson/parse/doc rejects with an extra field") {
    rule<bson::view> auto p = parse::doc(field("foo", type<std::int32_t>()));
    // The "foo" element is will not match, and no rule will match it
    auto doc = buildit(make::doc(pair("foo", "string")));
    auto r   = p(doc);
    CHECK_FALSE(did_accept(r));
    CHECK(describe_error(r) == "errors: [unexpected element ‘foo’]");
}
