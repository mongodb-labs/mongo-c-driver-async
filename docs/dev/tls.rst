###########
TLS Streams
###########

.. warning:: |devdocs-page|

Background
##########

In |amongoc|, the TLS engine is not tied to a socket. Instead, OpenSSL provides
a state machine :struct:`SSL` interface using a ``BIO`` as an abstraction around an
opaque I/O object. It doesn't concern itself with how bytes move in/out of that
BIO, it just expects it to behave like a stream of bytes.

Any I/O operation on the high-level TLS stream can be composed of an arbitrary
number of reads and writes to the actual underlying stream. As such, it is not
as simple as reading/writing data into the :struct:`SSL` and then receiving some
plaintext/cyphertext. A single read or write of plaintext may invoke more reads
or writes on the ciphertext stream, so the |amongoc| wrapper's read/write
operations need to be able to handle such a situation.

The stream wrapper in |amongoc| provides an asynchronous interface around that
BIO by moving bytes to/from the BIO from/to a wrapped `asio::AsyncWriteStream`.


Operation Implementation
########################

.. header-file::
    amongoc/tls/stream.hpp
    amongoc/tls/detail/stream_base.hpp

  **(Private headers)**

  Provides the implementation of a TLS stream wrapper. See:
  :class:`amongoc::tls::stream`

All operations are implemented with an abstract base class
`amongoc::tls::detail::stream_base::operation_base`, which provides a basic
interface:

- :cpp:`reenter(ec)` is called to pump the OpenSSL state machine. The error code
  given comes from the operation that reentered it. On initial reentry, this
  error code is always zero.
- :cpp:`complete(ec)` will be called exactly once for an operation and may
  destroy the operation object. An outstanding operation must complete before a
  new operation may be enqueued.
- If the error code given to :cpp:`reenter` is non-zero, then the operation
  immediately calls :cpp:`complete()` with that error.
- :cpp:`do_ssl_operation()` actually invokes the OpenSSL API for the operation.
  It may be called multiple times. It should return an integer for the result of
  the operation. It is called by :cpp:`reenter()`.
- After :cpp:`do_ssl_operation`, :cpp:`reenter` initiates more asynchronous I/O
  operations on the wrapped stream based on what the `SSL` engine needs.

  - If `SSL` wants more bytes, then the operation will initiate an async read.
    When the read completes, :cpp:`reenter` is called again and the received
    bytes are written into the `BIO` associated with the `SSL`.
  - If `SSL` has data to send, then the operation will pull bytes from the `BIO`
    and initiate an asynchronous write of those bytes on the stream.
  - If `SSL` reports an error, then :cpp:`complete` will be called with an error
    code.

- If `SSL` does not have any data to read or write, then the operation will
  consider itself to be complete, and it calls :cpp:`complete` with a success
  status.

Certificate Verification
########################

The certificate verification implementation in |amongoc| is based on the Asio
interface. There are two ways to verify certificates:

- Verification associated with the context: `asio::ssl::context` allows a
  verification callback to be attached to the context, and this will apply to
  all derived SSL engines. This relies entirely on the Asio implementation.
- Verification can be associated with a particular stream wrapper. This is
  implemented in |amongoc| but follows the same implementation pattern that Asio
  uses for stream-associated verification.

In either case, a verification callback is tied to an object using
:cpp:`set_verify_callback`. Refer to
`the Asio documentation <set_verify_callback_>`_ for information on how the
callback is invoked.

.. _set_verify_callback: https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/ssl__stream/set_verify_callback/overload1.html


Internal API
############

.. default-role:: any

.. class:: template <typename S> [[internal]] amongoc::tls::stream

  Implements an Asio `asio::AsyncWriteStream` that wraps another
  `asio::AsyncWriteStream` `S`.

  We cannot reuse the :cpp:`asio::ssl::stream<>`, because it requires access to
  a proper Asio execution context that we cannot provide with our event loop.

  Many of the APIs below are provided by the ``stream_base`` base class.

  .. function::
    stream(S&& s, asio::ssl::context& ctx)

    Construct a new stream. Perfect-forwards the stream `s` into place.

  .. function::
    S& next_layer()
    const S& next_layer() const

    Obtain the wrapped stream.

  .. function::
    ::SSL* ssl_cptr() const

    Obtain the pointer to the native OpenSSL engine object. This can be used to
    invoke OpenSSL APIs that are not otherwise exposed on the stream.

  .. function::
    std__error_code set_verify_mode(asio::ssl::verify_mode v)
    std__error_code set_verify_callback(auto fn)

    Set the verification mode and verification callback.

    `Refer to the Asio documentation for details <set_verify_callback_>`_

  .. function::
    std__error_code set_server_name(const std__string& sn)

    Set the server name for the peer. This is required if the peer uses SNI.

  .. function::
    nanosender_of<result<mlib::unit, std__error_code>> auto connect()

    Obtian a nanosender that performs a client-side handshake on the stream.

  .. function::
    void async_write_some(auto&& cbufs, auto&& callback)
    void async_read_some(auto&& mbufs, auto&& callback)

    Implement the `asio::AsyncReadStream` and `asio::AsyncWriteStream` interface
    for completion callbacks.

    This allows the `stream` wrapper to be used in higher-level interfaces.

    .. note::

      Don't use these directly. Instead, use the Asio
      :ref:`algorithms <asio.algo>`, which will allow you to use a proper
      `asio::CompletionToken` and have stronger behavioral guarantees.


.. struct:: SSL

  An OpenSSL TLS stream object. `Refer to the OpenSSL documentation`__

  __ https://docs.openssl.org/1.1.1/man3/SSL_new/


.. class:: asio::ssl::context

  We reuse Asio's wrapper around an OpenSSL context, as it implements a lot of
  useful functionality for us, and does not require access to a full Asio event
  loop.

  `Refer to the Asio documentation`__

  __ https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/ssl__context.html
