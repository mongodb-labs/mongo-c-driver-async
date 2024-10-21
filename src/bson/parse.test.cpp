#include <bson/doc.h>
#include <bson/make.hpp>
#include <bson/parse.hpp>
#include <bson/view.h>

#include <mlib/alloc.h>

#include <catch2/catch_test_macros.hpp>

using namespace bson;
using namespace bson::make;
using namespace bson::parse;

static document buildit(auto spec) { return spec.build(::mlib_default_allocator); }

TEST_CASE("bson/parse/doc") {
    // Field "bar" is optional, field "foo" is required
    std::int32_t          i = 0;
    rule<bson::view> auto d = parse::doc(field("bar", just_accept{}),
                                         must(field("foo", type<std::int32_t>(store(i)))),
                                         just_accept{});
    auto                  r = d(buildit(make::doc(pair("foo", 21))));
    CAPTURE(describe_error(r));
    CHECK(did_accept(r));
    CHECK(i == 21);
}

TEST_CASE("bson/parse/doc missing one required") {
    // "bar" and "foo" are both required
    rule<bson::view> auto d = parse::doc(must(field("bar", just_accept{})),
                                         must(field("foo", type<std::int32_t>())),
                                         just_accept{});
    auto                  r = d(buildit(make::doc(pair("bar", 21))));
    CAPTURE(describe_error(r));
    CHECK(describe_error(r) == "errors: [element ‘foo’ not found]");
    CHECK_FALSE(did_accept(r));
}

TEST_CASE("bson/parse/doc missing two required") {
    rule<bson::view> auto d = parse::doc(field("bar", just_accept{}),
                                         must(field("foo", type<std::int32_t>())),
                                         must(field("baz", type<std::int32_t>())),
                                         just_accept{});
    auto                  r = d(buildit(make::doc(pair("bar", 21))));
    CAPTURE(describe_error(r));
    CHECK(describe_error(r) == "errors: [element ‘foo’ not found, element ‘baz’ not found]");
    CHECK_FALSE(did_accept(r));
}

TEST_CASE("bson/parse/doc accepts missing non-required fields") {
    rule<bson::view> auto d = parse::doc(field("bar", just_accept{}),  //
                                         require("foo", type<std::int32_t>()));
    must_parse(buildit(make::doc(pair("foo", 42))), d);
}

TEST_CASE("bson/parse/doc optional field rejects, but accepts full doc") {
    rule<bson::view> auto d = parse::doc(field("bar", just_accept{}),
                                         field("foo", type<std::int32_t>()),
                                         just_accept{});
    auto                  r = d(buildit(make::doc(pair("foo", "string"))));
    CHECK(did_accept(r));
}

TEST_CASE("bson/parse/doc rejected optional field does not contribute error") {
    rule<bson::view> auto d = parse::doc(field("foo", type<std::int32_t>()),
                                         must(field("bar", just_accept{})),
                                         just_accept{});
    // The "foo" element will not be accepted because it has the wrong type, but
    // it should not contribute an error message because it is marked as optional
    auto r = d(buildit(make::doc(pair("foo", "string"))));
    CHECK_FALSE(did_accept(r));
    CHECK(describe_error(r) == "errors: [element ‘bar’ not found]");
}

TEST_CASE("bson/parse/doc rejects with optional field that generates a hard error") {
    rule<bson::view> auto d = parse::doc(field("foo", must(type<std::int32_t>())), just_accept{});
    // The "foo" element is optional, but if it appears then it must be an integer
    auto r = d(buildit(make::doc(pair("foo", "string"))));
    CHECK_FALSE(did_accept(r));
    CHECK(describe_error(r) == "errors: [in field ‘foo’: element has incorrect type]");
}

TEST_CASE("bson/parse/doc accepts with an extra field") {
    rule<bson::view> auto p = parse::doc(require("foo", type<std::int32_t>()));
    // The "foo" element is will not match, and no rule will match it
    auto doc = buildit(make::doc(pair("foo", 42), pair("bar", 42)));
    auto r   = p(doc);
    CHECK(did_accept(r));
}

TEST_CASE("bson/parse/doc rejects with an extra field when requested") {
    rule<bson::view> auto p = parse::doc(field("foo", type<std::int32_t>()), reject_others{});
    // The "foo" element is will not match, and no rule will match it
    auto doc = buildit(make::doc(pair("foo", 42), pair("bar", 1729)));
    auto r   = p(doc);
    CHECK_FALSE(did_accept(r));
    CHECK(describe_error(r) == "errors: [unexpected element ‘bar’]");
}

TEST_CASE("bson/parse/each/Custom errors") {
    auto rule
        = parse::doc(require("foo", type<bson_array_view>(each(integer([](auto i) -> basic_result {
                                 if (i % 2) {
                                     return {pstate::reject, "numbers must be even"};
                                 }
                                 return {};
                             })))));
    SECTION("All accept") {
        auto bson = buildit(make::doc(pair("foo", array(2, 4, 6, 8))));
        auto r    = rule(bson);
        CHECK(r.state() == pstate::accept);
        CHECK(describe_error(r) == "[accepted]");
    }

    SECTION("Bad Nth element") {
        auto bson = buildit(make::doc(pair("foo", array(2, 4, 3, 6, 8))));
        auto r    = rule(bson);
        CHECK(describe_error(r)
              == "errors: [in field ‘foo’: field ‘2’ was rejected: numbers must be even]");
    }

    SECTION("Bad type") {
        auto bson = buildit(make::doc(pair("foo", array(2, 4, "hey", 6, 8))));
        auto r    = rule(bson);
        CHECK(describe_error(r)
              == "errors: [in field ‘foo’: field ‘2’ was rejected: element does not have a numeric type]");
    }
}

TEST_CASE("bson/parse/each/maybe") {
    /// A parse rule:
    /// - Require an element `foo` of array type
    /// - For each element:
    ///   - It may be an integral
    ///   - If an integral, it must be an even number
    auto rule = parse::doc(
        require("foo", type<bson_array_view>(each(maybe(integer(must([](auto i) -> basic_result {
                    if (i % 2) {
                        return {pstate::reject, "numbers must be even"};
                    }
                    return {};
                })))))));

    SECTION("All accept") {
        auto bson = buildit(make::doc(pair("foo", array(2, 4, 6, 8))));
        auto r    = rule(bson);
        CHECK(r.state() == pstate::accept);
        CHECK(describe_error(r) == "[accepted]");
    }

    SECTION("Bad Nth element") {
        auto bson = buildit(make::doc(pair("foo", array(2, 4, 3, 6, 8))));
        auto r    = rule(bson);
        CHECK(describe_error(r)
              == "errors: [in field ‘foo’: field ‘2’ was rejected: numbers must be even]");
    }

    SECTION("Bad type") {
        auto bson = buildit(make::doc(pair("foo", array(2, 4, "hey", 6, 8))));
        auto r    = rule(bson);
        CHECK(describe_error(r) == "[accepted]");
    }
}

TEST_CASE("bson/parse/any/empty rejects") {
    auto rule = parse::any<>{};
    auto res  = rule(42);
    CHECK(res.state() == pstate::reject);
    CHECK(describe_error(res) == "[rejected]");
}

TEST_CASE("bson/parse/all/empty accepts") {
    rule<int> auto rule = parse::all<>{};
    auto           res  = rule(42);
    CHECK(res.state() == pstate::accept);
    CHECK(describe_error(res) == "[accepted]");
}
