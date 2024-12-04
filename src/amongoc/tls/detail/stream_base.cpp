#include "./stream_base.hpp"

#include <amongoc/status.h>

#include <asio/buffer.hpp>
#include <asio/ssl/verify_context.hpp>
#include <asio/ssl/verify_mode.hpp>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <cstddef>
#include <memory>
#include <system_error>

using namespace amongoc;
using namespace amongoc::tls;
using namespace amongoc::tls::detail;

void stream_base::operation_base::reenter(std::error_code ec) noexcept {
    if (ec) {
        this->complete(ec);
        return;
    }

    if (not _stream._pending_input.empty()) {
        // There is pending input from the underlying stream that the caller wants to feed to
        // OpenSSL
        auto n_given = ::BIO_write(_stream._bio.get(),
                                   _stream._pending_input.data(),
                                   _stream._pending_input.size());
        if (n_given < 0) {
            ec = std::error_code(::ERR_get_error(), tls_category());
            this->complete(ec);
            return;
        }
        _stream._pending_input.erase(0, static_cast<std::size_t>(n_given));
    }

    ::ERR_clear_error();
    const int  op_result = this->do_ssl_operation();
    const auto ssl_err   = ::SSL_get_error(_stream.ssl_cptr(), op_result);
    const int  eno       = errno;
    const auto sys_err   = static_cast<int>(::ERR_get_error());
    switch (ssl_err) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        // There is data to be transferred in the BIO
        break;

    case SSL_ERROR_NONE:
        // No error. The operation is complete successfully
        break;

    case SSL_ERROR_SYSCALL:
        // TODO: This should use GetLastError() on Windows
        ec = std::error_code(eno, std::system_category());
        break;
    case SSL_ERROR_SSL:
        ec = std::error_code(sys_err, amongoc::tls_category());
        break;
    case SSL_ERROR_ZERO_RETURN:
        // ??
        ec = std::make_error_code(std::errc::io_error);
        break;
    default:
        // Some other error?
        ec = std::make_error_code(std::errc::protocol_error);
        break;
    }
    if (ec) {
        return this->complete(ec);
    }

    if (auto n_pending = ::BIO_ctrl_pending(_stream._bio.get())) {
        // OpenSSL has some data that it wants to send to the peer
        auto dbuf = asio::dynamic_buffer(_stream._pending_output);
        auto buf  = dbuf.prepare(n_pending);
        // Move from the BIO into the output buffer that we will send
        auto nbytes [[maybe_unused]] = ::BIO_read(_stream._bio.get(), buf.data(), buf.size());
        assert(nbytes == static_cast<int>(n_pending));
        return _stream.do_write_some(io_callback{_stream, *this, false});
    } else if (SSL_want_read(_stream._ssl.get())) {
        // OpenSSL wants more data from the stream.
        _stream._pending_input.resize(1024);
        return _stream.do_read_some(io_callback{_stream, *this, true});
    } else {
        // Nothing to transmit. The operation is done
        return this->complete({ec});
    }
}

stream_base::stream_base(asio::ssl::context& ctx) {
    ::SSL_CTX* ssl_ctx = ctx.native_handle();
    ::ERR_clear_error();
    _ssl = ::SSL_new(ssl_ctx);
    if (not _ssl.get()) {
        throw std::system_error(std::error_code(::ERR_get_error(), tls_category()),
                                "Failed to create a new SSL engine");
    }

    ::SSL_set_mode(_ssl.get(), SSL_MODE_ENABLE_PARTIAL_WRITE);
    ::SSL_set_mode(_ssl.get(), SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    ::BIO* inner_bio = nullptr;
    ::BIO_new_bio_pair(&inner_bio, 0, &_bio.get(), 0);
    ::SSL_set_bio(_ssl.get(), inner_bio, inner_bio);
}

stream_base::~stream_base() {
    if (_ssl.get()) {
        delete static_cast<verify_callback_base*>(SSL_get_app_data(_ssl.get()));
    }
}

int stream_base::connect_op_base::do_ssl_operation() noexcept {
    return ::SSL_connect(_stream.ssl_cptr());
}

std::error_code stream_base::set_verify_mode(asio::ssl::verify_mode v) noexcept {
    ::SSL_set_verify(_ssl.get(), static_cast<int>(v), ::SSL_get_verify_callback(_ssl.get()));
    return {};
}

std::error_code stream_base::set_server_name(const std::string& sn) noexcept {
    int ec = ::SSL_set_tlsext_host_name(_ssl.get(), sn.data());
    if (ec == 0) {
        return std::error_code(::ERR_get_error(), tls_category());
    }
    return {};
}

std::error_code
stream_base::_replace_ssl_verify(std::unique_ptr<stream_base::verify_callback_base> cb) noexcept {
    // Delete the old callback
    delete static_cast<verify_callback_base*>(SSL_get_app_data(_ssl.get()));
    // Attach the new callback object
    SSL_set_app_data(_ssl.get(), cb.release());
    // The C callback:
    auto ssl_verifiy_cb = [](int preverified, ::X509_STORE_CTX* ctx) {
        if (not ctx) {
            // ???
            return 0;
        }
        // Get the SSL stream handle
        SSL* ssl = static_cast<::SSL*>(
            ::X509_STORE_CTX_get_ex_data(ctx, ::SSL_get_ex_data_X509_STORE_CTX_idx()));
        if (not ssl) {
            // ???
            return 0;
        }

        // Grab the callback object that is stored with the stream.
        auto cb_base = static_cast<verify_callback_base*>(SSL_get_app_data(ssl));
        if (not cb_base) {
            // ???
            return 0;
        }

        // Create the Asio verification context and invoke the stored callback
        asio::ssl::verify_context vtx{ctx};
        return cb_base->do_verify(preverified != 0, vtx);
    };
    // Attach the C callback
    ::SSL_set_verify(_ssl.get(), ::SSL_get_verify_mode(_ssl.get()), ssl_verifiy_cb);
    return {};
}
