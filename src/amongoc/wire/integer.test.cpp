#include <amongoc/wire/integer.hpp>

#include <asio/buffer.hpp>
#include <catch2/catch_test_macros.hpp>

#include <system_error>

using namespace std::literals;

TEST_CASE("amongoc/wire/Read integer from dbuf") {
    std::string s    = "\x1a\x2b\x3c\x4d\x5e\x6f";
    auto        dbuf = asio::dynamic_buffer(s);
    auto        i    = amongoc::wire::read_int_le<std::int32_t>(dbuf);
    CHECK(i == 0x4d3c2b1a);
    // Chars were removed from the buffer:
    CHECK(s == "\x5e\x6f");
    // Not enough chars remain, throws:
    CHECK_THROWS_AS(amongoc::wire::read_int_le<std::int32_t>(dbuf), std::system_error);
    // The buffer is unmodified:
    CHECK(s == "\x5e\x6f");
}

TEST_CASE("amongoc/wire/Write integer to dbuf") {
    std::string s;
    auto        dbuf = asio::dynamic_buffer(s);
    amongoc::wire::write_int_le(dbuf, std::int32_t(0x10203042));
    CHECK(s == "\x42\x30\x20\x10"sv);
}
