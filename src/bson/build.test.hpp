#pragma once

#include <bson/view.h>

#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <array>
#include <cstddef>

namespace bson::testing {

// View the bytes of a document as a range of char
inline auto bson_bytes_range(bson::view d) {
    auto c = reinterpret_cast<const char*>(d.data());
    return std::ranges::subrange(c, c + d.byte_size());
}

// Helper type for making in-situ static-sized character arrays
template <std::size_t N>
struct chars : std::array<char, N> {};

template <typename... Cs>
explicit chars(Cs...) -> chars<sizeof...(Cs)>;

// Check that the bytes of a BSON document are equal to the given range of char
#define CHECK_BSON_BYTES_EQ(B, ...)                                                                \
    CHECK_THAT(::bson_bytes_range(B), Catch::Matchers::RangeEquals(__VA_ARGS__))

}  // namespace bson::testing
