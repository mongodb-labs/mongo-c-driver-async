#pragma once

#include "./detail/stream_base.hpp"

#include <amongoc/wire/stream.hpp>

#include <mlib/object_t.hpp>

#include <asio/buffer.hpp>
#include <asio/completion_condition.hpp>
#include <openssl/err.h>

namespace amongoc::tls {

/**
 * @brief Wraps a writable stream with TLS functionality
 *
 * @tparam S The underlying stream
 */
template <wire::writable_stream S>
class stream : public detail::stream_base {
public:
    /**
     * @brief Construct a stream wrapper
     *
     * @param s The underlying stream object
     * @param ctx The OpenSSL context object to be used
     */
    explicit stream(S&& s, asio::ssl::context& ctx)
        : stream_base(ctx)
        , _stream(mlib_fwd(s)) {}

    // The executor. Required for Asio compatibility
    using executor_type = std::remove_cvref_t<S>::executor_type;

    /**
     * @brief Get the wrapped stream
     */
    S&       next_layer() noexcept { return mlib::unwrap_object(_stream); }
    const S& next_layer() const noexcept { return mlib::unwrap_object(_stream); }

private:
    // The wrapped stream
    mlib::object_t<S> _stream;

    // Write some data to the wrapped stream
    void do_write_some(io_callback cb) override {
        asio::async_write(this->next_layer(),
                          // Using dynamic_buffer will cause Asio to left-trim the bytes as they are
                          // transmitted
                          asio::dynamic_buffer(this->_pending_output),
                          asio::transfer_at_least(1),
                          cb);
    }

    // Read data from the wrapped stream
    void do_read_some(io_callback cb) override {
        asio::async_read(this->next_layer(),
                         // Use a non-dynamic buffer, as we don't want Asio to try and adjust the
                         // underlying string for us.
                         asio::buffer(this->_pending_input),
                         // Don't try to fill the buffer, just read at least one byte
                         asio::transfer_at_least(1),
                         cb);
    }
};

}  // namespace amongoc::tls
