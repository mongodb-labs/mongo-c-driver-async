#include <mlib/str.h>

#include <catch2/catch_test_macros.hpp>

#include <concepts>

static_assert(std::equality_comparable<mlib_str_view>);
static_assert(std::equality_comparable_with<mlib_str_view, std::string_view>);

static_assert(::mlib_str_at("bar", 0) == 'b');
static_assert(::mlib_str_at("bar", 1) == 'a');
static_assert(::mlib_str_at("bar", 2) == 'r');
static_assert(::mlib_str_at("bar", 3) == 0);
static_assert(::mlib_str_at("bar", -1) == 'r');
static_assert(::mlib_str_at("bar", -2) == 'a');
static_assert(::mlib_str_at("bar", -3) == 'b');

static_assert(::mlib_str_find("foo", "o") == 1);
static_assert(::mlib_str_find("foo", "g") < 0);
static_assert(::mlib_str_find("foo", "oo") == 1);
static_assert(::mlib_str_find("foo", "ooo") < 0);
static_assert(::mlib_str_find("foo", "foo") == 0);
static_assert(::mlib_str_find("foo", "") == 0);
static_assert(::mlib_str_find("foo", "f") == 0);
static_assert(::mlib_str_find("foo", "fooo") < 0);
static_assert(::mlib_str_find("", "fooo") < 0);
static_assert(::mlib_str_find("", "") == 0);

static_assert(::mlib_str_subview("foo", 0, 500) == "foo");
static_assert(::mlib_str_subview("foo", 1, 500) == "oo");
static_assert(::mlib_str_subview("foo", 1, 1) == "o");
static_assert(::mlib_str_subview("foo", 0, 1) == "f");
static_assert(::mlib_str_subview("foo", 0, 2) == "fo");
static_assert(::mlib_str_subview("foo", 3, 50) == "");

static_assert(::mlib_str_rfind("foo", "o") == 2);
static_assert(::mlib_str_rfind("foo", "g") < 0);
static_assert(::mlib_str_rfind("foo", "f") == 0);
static_assert(::mlib_str_rfind("foo", "fo") == 0);
static_assert(::mlib_str_rfind("foo", "oo") == 1);
static_assert(::mlib_str_rfind("foo", "ooo") < 0);
static_assert(::mlib_str_rfind("foo", "foo") == 0);
static_assert(::mlib_str_rfind("foo", "") == 3);
static_assert(::mlib_str_rfind("foo", "f") == 0);
static_assert(::mlib_str_rfind("foo", "fooo") < 0);
static_assert(::mlib_str_rfind("", "fooo") < 0);
static_assert(::mlib_str_rfind("", "") == 0);

TEST_CASE("mlib/string/new") {
    mlib_str s = mlib_str_new().str;
    mlib_str_delete(s);
}

TEST_CASE("mlib/string/resize") {
    mlib_str_mut s = mlib_str_new(16);
    mlib_str_mut_resize(&s, 1024);
    mlib_str_delete(s.str);
    s = mlib_str_copy("Hello!");
    CHECK(s == "Hello!");
    CHECK(mlib_strlen(s) == 6);
    mlib_str_mut_resize(&s, 2);
    CHECK(s == "He");
    mlib_str_delete(s.str);
}
