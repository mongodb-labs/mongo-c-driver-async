/**
 * @file buffer.hpp
 * @brief Buffer-oriented operation utilities
 * @date 2024-09-20
 *
 * The utilities in this file are intended for use with Asio buffer operations.
 */
#pragma once

#include <mlib/bytes.hpp>
#include <mlib/config.h>

#include <asio/buffer.hpp>
#include <asio/buffers_iterator.hpp>
#include <neo/iterator_facade.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>

namespace amongoc::wire {

/**
 * @brief Match a contiguous range of byte-sized objects
 */
template <typename T>
concept contiguous_byte_range = std::ranges::contiguous_range<T> and mlib::byte_range<T>;

// Match an iterator that yields `const_buffer` objects
template <typename T>
concept const_buffer_iterator = std::forward_iterator<T>
    and std::convertible_to<std::iter_reference_t<T>, asio::const_buffer>;

// Match an iterator that yields `mutable_buffer` objects
template <typename T>
concept mutable_buffer_iterator = const_buffer_iterator<T>
    and std::convertible_to<std::iter_reference_t<T>, asio::mutable_buffer>;

// Match an Asio ConstBufferSequence
template <typename T>
concept const_buffer_sequence = std::copy_constructible<T> and requires(T bufs) {
    { asio::buffer_sequence_begin(bufs) } -> const_buffer_iterator;
};

// Match an Asio MutableBufferSequence
template <typename T>
concept mutable_buffer_sequence = const_buffer_sequence<T> and requires(T bufs) {
    { asio::buffer_sequence_begin(bufs) } -> mutable_buffer_iterator;
};

// Match an Asio ConstBufferSequence that is actually just a single buffer
template <typename T>
concept single_const_buffer
    = const_buffer_sequence<T> and std::convertible_to<T, asio::const_buffer>;

// Match an Asio MutableBufferSequence that is actually just a single buffer
template <typename T>
concept single_mutable_buffer = mutable_buffer_sequence<T> and single_const_buffer<T>
    and std::convertible_to<T, asio::mutable_buffer>;

/**
 * @brief Match a type for Asio's DynamicBuffer_v1 concept
 */
template <typename T>
concept dynamic_buffer_v1 = requires(T dbuf, const std::size_t sz) {
    { dbuf.size() } -> std::same_as<std::size_t>;
    { dbuf.max_size() } -> std::same_as<std::size_t>;
    { dbuf.capacity() } -> std::same_as<std::size_t>;
    { dbuf.data() } -> const_buffer_sequence;
    { dbuf.prepare(sz) } -> mutable_buffer_sequence;
    { dbuf.commit(sz) };
    { dbuf.consume(sz) };
};

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
 * @brief Coerce an Asio buffer sequence into a standard C++ range type
 *
 * @param bufs A buffer sequence to be converted
 *
 * Asio supports buffer sequences that aren't standard ranges, based on the
 * `buffer_sequence_begin` / `buffer_sequence_end` mechanism. This function will coerce such buffer
 * sequence to regular ranges using the standard `begin` / `end` idiom.
 */
template <const_buffer_sequence B>
constexpr const_buffer_sequence decltype(auto) buffer_sequence_as_range(B&& bufs) {
    if constexpr (std::ranges::input_range<B>) {
        return mlib_fwd(bufs);
    } else {
        return std::ranges::subrange(asio::buffer_sequence_begin(bufs),
                                     asio::buffer_sequence_end(bufs));
    }
}

/**
 * @brief Create an byte-wise range that views the bytes of an Asio buffer sequence
 *
 * @param bufs the buffers to be viewed
 */
template <const_buffer_sequence B>
constexpr mlib::byte_range auto buffers_subrange(const B& bufs) {
    // Optimize for cases that the object is a single buffer.
    if constexpr (single_mutable_buffer<B>) {
        asio::mutable_buffer mb = bufs;
        auto                 p  = reinterpret_cast<char*>(mb.data());
        return std::ranges::subrange(p, p + mb.size());
    } else if constexpr (single_const_buffer<B>) {
        asio::const_buffer cb = bufs;
        auto               p  = reinterpret_cast<const char*>(cb.data());
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
constexpr mlib::byte_range auto buffers_unbounded(const B& bufs) {
    auto it = std::ranges::begin(buffers_subrange(bufs));
    return std::ranges::subrange(it, std::unreachable_sentinel);
}

}  // namespace amongoc::wire
