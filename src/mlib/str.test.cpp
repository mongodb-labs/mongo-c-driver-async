#include <mlib/str.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("mlib/string/new") {
    mlib_str s = mlib_str_new().str;
    mlib_str_delete(s);
}

TEST_CASE("mlib/string/resize") {
    mlib_str_mut s = mlib_str_new(16);
    mlib_str_mut_resize(&s, 1024);
    mlib_str_delete(s.str);
    s.str = mlib_str_copy("Hello!");
    CHECK(s == "Hello!");
    CHECK(mlib_str_length(s) == 6);
    mlib_str_mut_resize(&s, 2);
    CHECK(s == "He");
    mlib_str_delete(s.str);
}
