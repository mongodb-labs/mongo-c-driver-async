#include <mlib/bytes.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>
#include <system_error>

using namespace std::literals;
using str_iter = std::string_view::iterator;

TEST_CASE("mlib/bytes/Read int8-le") {
    auto                                         s = "\x1a\x2b"sv;
    mlib::decoded_integer<std::int8_t, str_iter> d = mlib::read_int_le<std::int8_t>(s);
    CHECK(d.value == 0x1a);
    CHECK(d.in == s.begin() + 1);
    s.remove_prefix(1);
    d = mlib::read_int_le<std::int8_t>(s);
    CHECK(d.value == 0x2b);
    CHECK(d.in == s.end());
}

TEST_CASE("mlib/bytes/Read int16-le") {
    auto                                          s = "\x1a\x2b\xff"sv;
    mlib::decoded_integer<std::int16_t, str_iter> d = mlib::read_int_le<std::int16_t>(s);
    CHECK(d.value == 0x2b1a);
    CHECK(d.in == s.begin() + 2);
    // Truncated read will throw:
    s.remove_prefix(2);
    CHECK_THROWS_AS(mlib::read_int_le<std::int16_t>(s), std::system_error);
}
