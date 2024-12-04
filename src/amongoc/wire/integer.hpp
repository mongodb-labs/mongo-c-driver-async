#pragma once

#include <amongoc/wire/buffer.hpp>

#include <mlib/bytes.hpp>

namespace amongoc::wire {

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
    mutable_buffer_sequence auto out     = dbuf.prepare(sizeof value);
    mlib::byte_range auto        out_rng = wire::bytes_of_buffers(out);
    write_int_le(out_rng, value);
    dbuf.commit(sizeof value);
}

/**
 * @brief Read a little-endian encoded integer from a dynamic buffer.
 *
 * @tparam I The integer type to be read
 * @param dbuf A dynamic buffer to be used. The bytes of the integer will be consumed
 */
template <typename I>
I read_int_le(dynamic_buffer_v1 auto&& dbuf) {
    const_buffer_sequence auto  data  = dbuf.data();
    mlib::byte_range auto const bytes = wire::bytes_of_buffers(data);
    I                           v     = mlib::read_int_le<I>(bytes).value;
    dbuf.consume(sizeof(I));
    return v;
}

}  // namespace amongoc::wire
