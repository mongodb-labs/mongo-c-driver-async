#include <mlib/integer.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

TEST_CASE("mlib/integer/make") {
    auto i = mlibMath(0);
    CHECK(i.i64 == 0);
}

TEST_CASE("mlib/integer/add") {
    auto i = mlibMath(U(21));
    i      = mlibMath(add(V(i), V(i)));
    CHECK(i.i64 == 42);
}

TEST_CASE("mlib/integer/add+overflow") {
    auto i = mlibMath(I((INT64_MAX / 2) + 4));
    i      = mlibMath(add(V(i), V(i)));
    CHECK(i.flags & mlib_integer_add_overflow);
    CHECK(i.i64 == (INT64_MIN + 6));
}

TEST_CASE("mlib/integer/catch") {
    mlib_math_try();
    auto i   = mlibMath(I(412));
    auto i64 = mlibMathInt32(mul(I(54), mul(512, mul(512, mul(512, mul(512, mul(512, 512)))))));
    mlib_math_catch (e) {
        CHECK(e.flags == mlib_integer_bounds);
        return;
    }
    FAIL_CHECK("Should not be reached");
}
