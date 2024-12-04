#pragma once

#include <amongoc/nano/result.hpp>
#include <amongoc/wire/buffer.hpp>

#include <mlib/config.h>
#include <mlib/delete.h>
#include <mlib/object_t.hpp>
#include <mlib/unique.hpp>

#include <asio/buffer.hpp>
#include <asio/ssl/context.hpp>
#include <openssl/ssl.h>

#include <iterator>
#include <system_error>

// Tell object deletion how to delete SSL objects
mlib_assoc_deleter(::SSL*, ::SSL_free);
mlib_assoc_deleter(::BIO*, ::BIO_free);

namespace amongoc::tls::detail {

/**
 * @brief Abstract base class for OpenSSL stream wrappers. Derived by `amongoc::tls::stream<>`
 */
class stream_base {
public:
    /**
     * @brief Construct the stream base with the OpenSSL context
     *
     * @param ctx A valid OpenSSL context. Must outlive the stream
     */
    explicit stream_base(asio::ssl::context& ctx);
    // Deletes the appdata associated with the SSL context
    virtual ~stream_base();

    stream_base(stream_base&&)            = default;
    stream_base& operator=(stream_base&&) = default;

    /**
     * @brief Obtain the handle to the `::SSL` engine object underlying the stream
     */
    ::SSL* ssl_cptr() const noexcept { return _ssl.get(); }

    /**
     * @brief Set the peer verification mode for a `connect` on the stream
     *
     * @param v The verification mode. See the Asio docs for details
     *
     * Should be called before `connect`.
     */
    [[nodiscard]] std::error_code set_verify_mode(asio::ssl::verify_mode v) noexcept;

    /**
     * @brief Set the certificate verification callback
     *
     * @param fn The callback to be attached to the stream. Perfect-forwarded
     * into place.
     *
     * See the Asio docs for `asio::ssl::stream::set_verify_callback`.
     * Should be called before `connect`.
     */
    template <typename F>
    [[nodiscard]] std::error_code set_verify_callback(F fn) noexcept {
        return _replace_ssl_verify(std::make_unique<verify_callback<F>>(mlib_fwd(fn)));
    }

    /**
     * @brief Set the expected server name on the peer. Required for SNI
     * certificate verification.
     *
     * Should be called before `connect`
     */
    [[nodiscard]] std::error_code set_server_name(const std::string&) noexcept;

    /**
     * @brief Create a nanosender that performs an OpenSSL client handshake.
     */
    auto connect() noexcept { return _connect_sender{this}; }

    /**
     * @brief Asio AsyncWriteStream interface
     *
     * @param bufs The buffers to be transmitted
     * @param cb The callback for when the operation completes
     */
    template <typename Buffers, typename Cb>
    void async_write_some(Buffers bufs, Cb&& cb) {
        _assert_no_outstanding_io();
        _live_operation
            = std::make_unique<write_operation<Buffers, Cb>>(*this, mlib_fwd(bufs), mlib_fwd(cb));
        _live_operation->reenter({});
    }

    /**
     * @brief Asio AsyncReadStream interface
     *
     * @param bufs The buffers to be filled
     * @param cb The completion callback for the operation
     */
    template <typename Buffers, typename Cb>
    void async_read_some(Buffers bufs, Cb&& cb) {
        _assert_no_outstanding_io();
        _live_operation
            = std::make_unique<read_operation<Buffers, Cb>>(*this, mlib_fwd(bufs), mlib_fwd(cb));
        _live_operation->reenter({});
    }

private:
    /**
     * @brief Asserts that there are no outstanding I/O operations on the stream.
     *
     * Terminates if there are outstanding operations.
     */
    void _assert_no_outstanding_io() const noexcept {
        if (_live_operation) {
            std::fprintf(
                stderr,
                "Attempted to enqueue TLS I/O while an outstanding I/O operation is live\n");
            std::terminate();
        }
    }

    /**
     * @brief Abstract base class that stores a callback for certificate verification
     */
    struct verify_callback_base {
        virtual ~verify_callback_base() = default;
        // Implements the verification. Should return an OpenSSL error code
        virtual int do_verify(bool is_preverified, asio::ssl::verify_context& ctx) = 0;
    };

    /**
     * @brief Implementation that stores a verification callback
     */
    template <typename F>
    struct verify_callback : verify_callback_base {
        explicit verify_callback(F&& fn)
            : _fn(fn) {}

        // The stored callback function. May be a reference
        mlib_no_unique_address F _fn;

        // Invokes the callback
        int do_verify(bool is_preverified, asio::ssl::verify_context& ctx) override {
            return _fn(is_preverified, ctx);
        }
    };

    // Replace the SSL verification callback associated with the stream.
    std::error_code _replace_ssl_verify(std::unique_ptr<verify_callback_base> cb) noexcept;

private:
    /**
     * @brief This BIO object is the input/output stream for the SSL engine.
     *
     * When cyphertext is received from the peer, we write that data into this BIO.
     * When OpenSSL has more cyphertext to be sent to the peer, it should be pulled
     * from this BIO.
     */
    mlib::unique<::BIO*> _bio;
    /**
     * @brief The OpenSSL state machine associated with this stream.
     */
    mlib::unique<::SSL*> _ssl;

    /**
     * @brief Abstract base class for OpenSSL operation state
     *
     * An operation object stores a reference to the stream from which it was
     * created. Operation objects should not be moved once they are initiated.
     *
     * The operation state for connect() is a nanooperation state, but the read/write
     * operations are only used by Asio, so they implement a slightly different
     * interface.
     */
    struct operation_base {
        // Store a reference to the stream object
        explicit operation_base(stream_base& stream)
            : _stream{stream} {}

        // Virtual dtor allows deletion through a base class pointer
        virtual ~operation_base() = default;

        /**
         * @brief Called to invoke the attached receiver/callback when the operation
         * completes.
         */
        void complete(std::error_code ec) {
            // Detach the oustanding operation from the stream, allowing a new operation to be
            // enqueued by the completion handler if necessary. This pointer may be null, but
            // we don't actually need to look at it here.
            auto taken = std::move(_stream._live_operation);
            this->do_complete(ec);
        }

        /**
         * @brief Invoked by the event loop when async I/O completes. See stream_base.cpp for
         * details.
         *
         * @param ec An error code for the partial operation.
         */
        void reenter(std::error_code ec) noexcept;

    protected:
        // The associated stream object. Must be stable during the operation
        stream_base& _stream;

        // Invoke the associated handler/receiver
        virtual void do_complete(std::error_code) = 0;

        // Callback that invokes the OpenSSL API that tries the operation.
        virtual int do_ssl_operation() noexcept = 0;
    };

    /**
     * @brief Pointer to an outstanding operation object.
     *
     * This is only used by read/write operations since Asio gives us no place
     * to store the operation state otherwise. The connection operation can use
     * a nanosender API to store the stable operation in the caller's scope.
     *
     * This gives us the bonus ability to detect when an I/O operation is live, since
     * this pointer will be non-null. We can use this to detect attempts at overlapping
     * I/O and terminate the program since such behavior is undefined.
     */
    std::unique_ptr<operation_base> _live_operation;

    // Abstract connect operation.
    class connect_op_base : public operation_base {
    protected:
        using operation_base::operation_base;

    public:
        // Reenter the operation for the first time.
        void start() noexcept { this->reenter({}); }

        // Calls ::SSL_connect
        int do_ssl_operation() noexcept override;
    };

    // nanosender for connect()
    struct _connect_sender {
        stream_base* _self;
        using sends_type = result<mlib::unit, std::error_code>;

        template <nanoreceiver_of<sends_type> R>
        nanooperation auto connect(R&& recv) const {
            return _connect_op<R>{{*_self}, mlib_fwd(recv)};
        }
    };

    // Connect operation impl that stores the receiver
    template <nanoreceiver_of<_connect_sender::sends_type> R>
    struct _connect_op : connect_op_base {
        explicit _connect_op(stream_base& self, R&& recv)
            : connect_op_base(self)
            , _recv(mlib_fwd(recv)) {}

        // Receiver for the connect operation
        mlib_no_unique_address R _recv;

        void do_complete(std::error_code ec) override {
            if (ec) {
                mlib::invoke(static_cast<R&&>(_recv), _connect_sender::sends_type(error(ec)));
            } else {
                mlib::invoke(static_cast<R&&>(_recv), _connect_sender::sends_type(success()));
            }
        }
    };

    // Abstract base for data transfer operations
    struct transfer_operation_base : operation_base {
        using operation_base::operation_base;
        // Cumulative number of bytes that have been transferred
        std::size_t _n_transferred = 0;

        /**
         * @brief Attempt to perform more partial I/O on the stream.
         *
         * @param bufseq The buffer sequence
         * @param op The transfer operation being executed.
         * @param ssl_transfer The OpenSSL function pointer that performs I/O
         * @return int The number of bytes sent, or zero, or a negative SSL error
         */
        int _transfer_more(auto bufseq, auto ssl_transfer) {
            auto one_buffer = this->first_nonempty(bufseq);
            if (one_buffer.size() == 0) {
                // No more data. We are done
                return 0;
            }
            // Invoke the transfer
            int n_transfer = ssl_transfer(_stream.ssl_cptr(),
                                          one_buffer.data(),
                                          static_cast<int>(one_buffer.size()));
            if (n_transfer > 0) {
                // We moved more bytes. Increment our byte counter
                this->_n_transferred += static_cast<std::size_t>(n_transfer);
            }
            // Return the number of bytes, or an error code.
            return n_transfer;
        }

        /**
         * @brief Obtain the first non-empty buffer in the sequence that we haven't sent according
         * to _n_transferred
         *
         * @param bufseq A buffer sequence for the whole operation
         * @return auto A single contiguous buffer of unset data, or any empty buffer if we are done
         */
        auto first_nonempty(auto bufseq) const noexcept {
            // The number of bytes into the buffer sequence that we should skip
            std::size_t remaining_skip = this->_n_transferred;
            // Iterate the buffer range
            auto       iter = asio::buffer_sequence_begin(bufseq);
            const auto stop = asio::buffer_sequence_end(bufseq);
            for (; iter != stop; ++iter) {
                // Grab one buffer
                auto one_buffer = *iter;
                // Check if we have already sent the entire buffer
                if (remaining_skip >= one_buffer.size()) {
                    // We skip this buffer entirely.
                    remaining_skip -= one_buffer.size();
                    continue;
                }
                // We haven't sent this full buffer yet, but we may have sent it partially:
                one_buffer += remaining_skip;
                return one_buffer;
            }
            // No more buffers to send. Return an empty buffer to indicate completion
            return std::iter_value_t<decltype(iter)>();
        }
    };

    /**
     * @brief Partial transfer operation state that stores the buffers and callback
     *
     * @tparam Bufs The buffer sequence to be transferred
     * @tparam F The Asio callback for the operation
     */
    template <typename Bufs, typename F>
    struct transfer_operation_partial : transfer_operation_base {
        explicit transfer_operation_partial(stream_base& self, Bufs bufs, F&& fn)
            : transfer_operation_base(self)
            , _buffers(bufs)
            , _fn(mlib_fwd(fn)) {}

        Bufs _buffers;
        F    _fn;

        void do_complete(std::error_code ec) override {
            // We are invoking an Asio-style I/O callback
            mlib::invoke(static_cast<F&&>(_fn), ec, this->_n_transferred);
        }
    };

    // Implements the operation state for a read
    template <typename Bufs, typename F>
    struct read_operation : transfer_operation_partial<Bufs, F> {
        using read_operation::transfer_operation_partial::transfer_operation_partial;
        int do_ssl_operation() noexcept override {
            return this->_transfer_more(this->_buffers, &::SSL_read);
        }
    };

    // Implements the operation state for a write
    template <typename Bufs, typename F>
    struct write_operation : transfer_operation_partial<Bufs, F> {
        using write_operation::transfer_operation_partial::transfer_operation_partial;
        int do_ssl_operation() noexcept override {
            return this->_transfer_more(this->_buffers, &::SSL_write);
        }
    };

protected:
    /**
     * @brief Asio callback object that reenters an asynchronous operation when
     * it is completed.
     */
    struct io_callback {
        stream_base&    self;
        operation_base& op;
        bool            is_read;

        void operator()(asio::error_code ec, std::size_t nbytes) {
            if (is_read) {
                // Truncate to the data that we actually read into the buffer
                self._pending_input.resize(nbytes);
            }
            op.reenter(ec);
        }
    };

    // Data that needs to be written into the wrapped stream
    std::string _pending_output;
    // Storage for data to be read from the wrapped stream
    std::string _pending_input;

    /**
     * @brief Push data to the wrapped I/O stream. This should send data from
     * the _pending_output string
     */
    virtual void do_write_some(io_callback) = 0;
    /**
     * @brief Push data to the wrapped I/O stream. This should read data into
     * the _pending_input string.
     */
    virtual void do_read_some(io_callback) = 0;
};

}  // namespace amongoc::tls::detail
