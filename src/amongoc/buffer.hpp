#pragma once

#include <amongoc/asio/read_write.hpp>

#include <mlib/config.h>

#include <asio/buffer.hpp>
#include <asio/buffers_iterator.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>

namespace amongoc {

// Match a range whose objects are byte-sized
template <typename T>
concept byte_range = std::ranges::input_range<T> and sizeof(std::ranges::range_value_t<T>) == 1
    and requires(std::ranges::range_reference_t<T> b) { static_cast<std::byte>(b); };

template <typename T>
concept contiguous_byte_range = std::ranges::contiguous_range<T> and byte_range<T>;

/**
 * @brief Provides an Asio DynamicBufferv1 interface over a generic array-like
 * object, but never allocating or growing the underlying range
 *
 * @tparam T A contiguous range of bytes
 */
template <contiguous_byte_range T>
class generic_dynamic_buffer_v1 {
public:
    using const_buffers_type   = asio::const_buffer;
    using mutable_buffers_type = asio::mutable_buffer;

    generic_dynamic_buffer_v1() = default;

    constexpr explicit generic_dynamic_buffer_v1(T&& buf, std::size_t ready_size = 0)
        : _buffer(mlib_fwd(buf))
        , _output_offset(ready_size) {}

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
explicit generic_dynamic_buffer_v1(T&&, std::size_t = 0) -> generic_dynamic_buffer_v1<T>;

/**
 * @brief Create an byte-wise range that views the bytes of a buffer sequence
 *
 * @param bufs the buffers to be viewed
 */
template <const_buffer_sequence B>
constexpr byte_range auto buffers_subrange(const B& bufs) {
    // Optimize for cases that the object is a single buffer.
    if constexpr (std::convertible_to<B, asio::mutable_buffer>) {
        auto mb = asio::mutable_buffer(bufs);
        auto p  = reinterpret_cast<char*>(mb.data());
        return std::ranges::subrange(p, p + mb.size());
    } else if constexpr (std::convertible_to<B, asio::const_buffer>) {
        auto cb = asio::const_buffer(bufs);
        auto p  = reinterpret_cast<const char*>(cb.data());
        return std::ranges::subrange(p, p + cb.size());
    } else {
        return std::ranges::subrange(asio::buffers_begin(bufs), asio::buffers_end(bufs));
    }
}

/**
 * @brief Create a byte-wise range from a buffer sequence, without a bounds-checking iterator
 *
 * @param bufs The buffers to be viewed
 *
 * @note Only use this for operations that are guaranteed to never overrun the buffer range
 */
template <const_buffer_sequence B>
constexpr byte_range auto buffers_unbounded(const B& bufs) {
    auto it = std::ranges::begin(buffers_subrange(bufs));
    return std::ranges::subrange(it, std::unreachable_sentinel);
}

}  // namespace amongoc
