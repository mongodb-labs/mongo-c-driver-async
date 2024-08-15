#pragma once

#include "./as_sender.hpp"

#include <asio/error_code.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

namespace amongoc {

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
