#pragma once

#include "./as_sender.hpp"

#include <asio/error_code.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include <concepts>
#include <iterator>

namespace amongoc {

template <typename T>
concept const_buffer_iterator = std::forward_iterator<T>
    and std::convertible_to<std::iter_reference_t<T>, asio::const_buffer>;

template <typename T>
concept mutable_buffer_iterator = const_buffer_iterator<T>
    and std::convertible_to<std::iter_reference_t<T>, asio::mutable_buffer>;

template <typename T>
concept const_buffer_sequence = std::copy_constructible<T> and requires(T bufs) {
    { asio::buffer_sequence_begin(bufs) } -> const_buffer_iterator;
};

template <typename T>
concept mutable_buffer_sequence = const_buffer_sequence<T> and requires(T bufs) {
    { asio::buffer_sequence_begin(bufs) } -> mutable_buffer_iterator;
};

template <typename T, typename Bufs = asio::mutable_buffer>
concept readable_stream = requires(T& strm, Bufs bufs) {
    {
        asio::async_read(strm, bufs, asio_as_nanosender)
    } -> nanosender_of<result<std::size_t, asio::error_code>>;
};

template <typename T, typename Bufs = asio::const_buffer>
concept writable_stream = requires(T& strm, Bufs bufs) {
    {
        asio::async_write(strm, bufs, asio_as_nanosender)
    } -> nanosender_of<result<std::size_t, asio::error_code>>;
};

}  // namespace amongoc
