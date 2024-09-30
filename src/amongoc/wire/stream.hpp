#pragma once

#include <amongoc/asio/as_sender.hpp>

#include <asio/read.hpp>
#include <asio/write.hpp>

namespace amongoc::wire {

template <typename T, typename Bufs = asio::mutable_buffers_1>
concept readable_stream = requires(T& strm, Bufs bufs) {
    {
        asio::async_read(strm, bufs, asio_as_nanosender)
    } -> nanosender_of<result<std::size_t, asio::error_code>>;
};

template <typename T, typename Bufs = asio::const_buffers_1>
concept writable_stream = readable_stream<T> and requires(T& strm, Bufs bufs) {
    {
        asio::async_write(strm, bufs, asio_as_nanosender)
    } -> nanosender_of<result<std::size_t, asio::error_code>>;
};

}  // namespace amongoc::wire
