#include <bson/doc.h>
#include <bson/iterator.h>
#include <bson/value.h>
#include <bson/value_ref.h>

#include <mlib/alloc.h>
#include <mlib/str.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("bson/value/bson_value_ref_from") {
    auto v = ::bson_value_ref_from(3.1);
    CHECK(v.type == ::bson_type_double);

    v = ::bson_value_ref_from(31);
    CHECK(v.type == ::bson_type_int32);
}

TEST_CASE("bson/value/copy+delete") {
    std::string     u8 = "Howdy";
    bson::document  doc{::mlib_default_allocator};
    bson_datetime   dt{};
    bson_regex_view rx = {
        .regex   = mlib_str_view_from("hello"),
        .options = mlib_str_view_from("xi"),
    };
    bson_dbpointer_view dbp;
    bson_oid            oid{};
    dbp.collection = mlib_str_view_from(u8);
    dbp.object_id  = oid;
    bson_code_view code;
    code.utf8 = mlib_str_view_from(u8);
    bson_symbol_view sym;
    sym.utf8 = mlib_str_view_from(u8);
    bson_timestamp  ts{};
    bson_decimal128 dec{};
    bson_value_ref_from(3.1);
    bson_value_ref_from(3.1f);
    bson_value_ref_from(u8);
    bson_value_ref_from("hello");
    bson_value_ref_from(doc);
    bson_value_ref_from(true);
    bson_value_ref_from(dt);
    bson_value_ref_from(rx);
    bson_value_ref_from(dbp);
    bson_value_ref_from(code);
    bson_value_ref_from(sym);
    bson_value_ref_from((int32_t)42);
    bson_value_ref_from(ts);
    bson_value_ref_from((int64_t)42);
    bson_value_ref_from(dec);

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
    auto val = bson_value_ref_from("string");
    CHECK(val != 41);
    CHECK(val == "string");
}
