#include <bson/doc.h>
#include <bson/iterator.h>
#include <bson/value.h>
#include <bson/value_ref.h>

#include <mlib/alloc.h>
#include <mlib/str.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("bson/value/bson_as_value_ref") {
    auto v = ::bson_as_value_ref(3.1);
    CHECK(v.type == ::bson_type_double);

    v = ::bson_as_value_ref(31);
    CHECK(v.type == ::bson_type_int32);
}

TEST_CASE("bson/value/copy+delete") {
    std::string     u8 = "Howdy";
    bson::document  doc{::mlib_default_allocator};
    bson_datetime   dt{};
    bson_regex_view rx = {
        .regex   = mlib_as_str_view("hello"),
        .options = mlib_as_str_view("xi"),
    };
    bson_dbpointer_view dbp;
    bson_oid            oid{};
    dbp.collection = mlib_as_str_view(u8);
    dbp.object_id  = oid;
    bson_code_view code;
    code.utf8 = mlib_as_str_view(u8);
    bson_symbol_view sym;
    sym.utf8 = mlib_as_str_view(u8);
    bson_timestamp  ts{};
    bson_decimal128 dec{};
    bson_as_value_ref(3.1);
    bson_as_value_ref(3.1f);
    bson_as_value_ref(u8);
    bson_as_value_ref("hello");
    bson_as_value_ref(doc);
    bson_as_value_ref(true);
    bson_as_value_ref(dt);
    bson_as_value_ref(rx);
    bson_as_value_ref(dbp);
    bson_as_value_ref(code);
    bson_as_value_ref(sym);
    bson_as_value_ref((int32_t)42);
    bson_as_value_ref(ts);
    bson_as_value_ref((int64_t)42);
    bson_as_value_ref(dec);

    bson_value_delete(bson_value_copy(3.1));
    bson_value_delete(bson_value_copy(3.1f));
    bson_value_delete(bson_value_copy(u8));
    bson_value_delete(bson_value_copy("Hello"));
    bson_value_delete(bson_value_copy(doc));
    bson_value_delete(bson_value_copy(true));
    bson_value_delete(bson_value_copy(dt));
    bson_value_delete(bson_value_copy(rx));
    bson_value_delete(bson_value_copy(dbp));
    bson_value_delete(bson_value_copy(code));
    bson_value_delete(bson_value_copy(sym));
    bson_value_delete(bson_value_copy((int32_t)42));
    bson_value_delete(bson_value_copy(ts));
    bson_value_delete(bson_value_copy((int64_t)42));
    bson_value_delete(bson_value_copy(dec));
}

TEST_CASE("bson/value_ref/compare") {
    auto val = bson_as_value_ref("string");
    CHECK(val != 41);
    CHECK(val == "string");
}
