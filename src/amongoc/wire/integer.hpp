#pragma once

#include <amongoc/wire/buffer.hpp>

namespace amongoc::wire {

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
 * @brief Write a little-endian encoded integer to the given output range
 *
 * @param out The destination for the encoded integer
 * @param value The integral value to be encoded
 * @return O The new position of the output iterator
 */
template <std::integral I, std::ranges::output_range<char> O>
constexpr std::ranges::iterator_t<O> write_int_le(O&& out_rng, I value) {
    // Make unsigned (ensures two's-complement for valid bit-shifting)
    const auto  uv   = static_cast<std::make_unsigned_t<I>>(value);
    auto        out  = std::ranges::begin(out_rng);
    const auto  stop = std::ranges::end(out_rng);
    std::size_t n    = 0;
    // Copy each byte of the integer in LE-order
    for (; n < sizeof uv and out != stop; ++n, ++out) {
        *out = static_cast<char>((uv >> (8 * n)) & 0xff);
    }
    if (n < sizeof uv) {
        throw std::system_error(std::make_error_code(std::errc::protocol_error), "short read");
    }
    return out;
}

/**
 * @brief Write a little-endian encoded integer to the given dynamic buffer output
 *
 * @param dbuf A dynamic buffer which will receive the encoded integer
 * @param value The value to be encoded
 */
template <std::integral I>
void write_int_le(dynamic_buffer_v1 auto&& dbuf, I value) {
    auto out     = dbuf.prepare(sizeof value);
    auto out_rng = buffers_subrange(out);
    write_int_le(out_rng, value);
    dbuf.commit(sizeof value);
}

/**
 * @brief Read a little-endian encoded integer from the given byte range
 *
 * @tparam Int The integer type to be read
 * @param it The input iterator from which we will read
 * @return A pair of the integer value and final iterator position
 */
template <typename Int, byte_range R>
constexpr decoded_integer<Int, std::ranges::iterator_t<R>> read_int_le(R&& rng) {
    using U          = std::make_unsigned_t<Int>;
    U           u    = 0;
    auto        it   = std::ranges::begin(rng);
    const auto  stop = std::ranges::end(rng);
    std::size_t n    = 0;
    for (; n < sizeof u and it != stop; ++it, ++n) {
        // Cast to unsigned byte first to prevent a sign-extension
        U b = static_cast<std::uint8_t>(*it);
        b <<= (8 * n);
        u |= b;
    }
    if (n < sizeof u) {
        // We had to stop before reading the full data
        throw std::system_error(std::make_error_code(std::errc::protocol_error), "short read");
    }
    return {static_cast<Int>(u), it};
}

/**
 * @brief Read a little-endian encoded integer from a dynamic buffer.
 *
 * @tparam I The integer type to be read
 * @param dbuf A dynamic buffer to be used. The bytes of the integer will be consumed
 */
template <typename I>
I read_int_le(dynamic_buffer_v1 auto&& dbuf) {
    const_buffer_sequence auto data = dbuf.data();
    auto                       subr = buffers_subrange(data);
    I                          v    = read_int_le<I>(subr).value;
    dbuf.consume(sizeof(I));
    return v;
}

}  // namespace amongoc::wire
