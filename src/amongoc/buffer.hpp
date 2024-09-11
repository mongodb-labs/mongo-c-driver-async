#pragma once

#include <mlib/config.h>

#include <asio/buffer.hpp>

namespace amongoc {

template <typename T>
concept contiguous_byte_container
    = std::ranges::contiguous_range<T> and sizeof(std::ranges::range_value_t<T>) == 1;

/**
 * @brief Provides an Asio DynamicBufferv1 interface over a generic array-like
 * object, but never allocating
 *
 * @tparam T
 * @param buf
 * @return requires
 */
template <contiguous_byte_container T>
class generic_dynamic_buffer_v1 {
public:
    using const_buffers_type   = asio::const_buffer;
    using mutable_buffers_type = asio::mutable_buffer;

    constexpr explicit generic_dynamic_buffer_v1(T&& buf)
        : _buffer(mlib_fwd(buf)) {}

    // Get the size of input area
    constexpr std::size_t size() const noexcept { return _output_offset - _input_offset; }
    constexpr std::size_t max_size() const noexcept { return std::ranges::size(_buffer); }
    constexpr std::size_t capacity() const noexcept { return size(); }

    // Get a buffer for the input area
    const_buffers_type data() const noexcept {
        return asio::const_buffer(std::ranges::data(_buffer) + _input_offset, size());
    }
    // Get a buffer of size `n` for the output area
    mutable_buffers_type prepare(std::size_t n) const noexcept {
        return asio::mutable_buffer(std::ranges::data(_buffer) + _output_offset, n);
    }
    // Move bytes from the output to the input area
    constexpr void commit(std::size_t n) noexcept { _output_offset += n; }
    // Discard from the beginning of the input area
    constexpr void consume(std::size_t n) noexcept { _input_offset += n; }

private:
    T _buffer;
    // Offset of the beginning of the input area
    std::size_t _input_offset = 0;
    // Offset of the beginning of the output area
    std::size_t _output_offset = 0;
};

template <typename T>
explicit generic_dynamic_buffer_v1(T&&) -> generic_dynamic_buffer_v1<T>;

}  // namespace amongoc
