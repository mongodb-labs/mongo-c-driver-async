#pragma once

#include <cstdint>
#include <ranges>
#include <type_traits>

namespace mlib {

/**
 * @brief Match an iterator that yields byte-sized objects that are explicit-convertible to
 * `std::byte`
 */
template <typename T>
concept byte_iterator =
    // Must be an iterator
    std::input_iterator<T>
    // Value type must be byte-sized
    and sizeof(std::iter_value_t<T>) == 1
    // Reference must be explicit-convertible to `std::byte`
    and requires(std::iter_reference_t<T> b) { static_cast<std::byte>(b); };

/**
 * @brief Match a range whose value type is byte-sized and convertible to `std::byte`
 *
 * This range is not necessarily contiguous
 */
template <typename T>
concept byte_range = std::ranges::input_range<T> and byte_iterator<std::ranges::iterator_t<T>>;

template <typename T>
concept forward_byte_range = byte_range<T> and std::ranges::forward_range<T>;

/**
 * @brief Result of decoding an integer from an input range
 *
 * @tparam I The integer value type
 * @tparam Iter The iterator type
 */
template <typename I, typename Iter>
struct decoded_integer {
    // The decoded integer value
    I value;
    // The input iterator position after decoding is complete
    Iter in;
};

/**
 * @brief Read a little-endian encoded integer from the given byte range
 *
 * @tparam Int The integer type to be read
 * @param it The input iterator from which we will read
 * @return A pair of the integer value and final iterator position
 */
template <typename Int, mlib::byte_range R>
constexpr decoded_integer<Int, std::ranges::iterator_t<R>> read_int_le(R&& rng) {
    using U          = std::make_unsigned_t<Int>;
    U           u    = 0;
    auto        it   = std::ranges::begin(rng);
    const auto  stop = std::ranges::end(rng);
    std::size_t n    = 0;
    for (; n < sizeof u and it != stop; ++it, ++n) {
        // Cast to unsigned byte first to prevent a sign-extension
        U b = static_cast<std::uint8_t>(static_cast<std::byte>(*it));
        b <<= (8 * n);
        u |= b;
    }
    if (n < sizeof u) {
        // We had to stop before reading the full data
        throw std::system_error(std::make_error_code(std::errc::protocol_error), "short read");
    }
    return {static_cast<Int>(u), it};
}

}  // namespace mlib
