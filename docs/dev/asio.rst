#################
Asio in |amongoc|
#################

.. warning:: |devdocs-page|

This page will explain the basics of Asio_ and how it is used internally in
|amongoc|.

.. _Asio: https://think-async.com/

Asio vs Boost.Asio
##################

Asio comes in two flavors: Boost, and non-Boost (i.e. "Standalone").

Boost.Asio is an automatically-converted version of Asio that has all APIs moved
from ``::asio`` to the ``::boost::asio`` namespace, and makes use of some Boost
libraries in place of standard library components. The following differences are
of note:

1. All components in :cpp:`::asio::` are in :cpp:`::boost::asio::`
2. Asio accepts an :cpp:`asio::error_code`, which is a typedef to
   `std__error_code` in standalone Asio, and a typedef of
   :cpp:`boost::system::error_code` in Boost.Asio.\ [#fn-sys-link]_


The Asio Asynchrony Model
#########################

Asio asynchrony is created using asynchronous APIs that accept an object known
as a `CompletionToken <asio::CompletionToken>`, which informs the operation how
to initiate and how to complete.

Asio's async API's can be further broken down into two classes: I/O *objects*,
and asynchronous *algorithms*.

.. _asio.io-objs:

I/O Objects
***********

Asio I/O objects are objects that abstract the implementaion of an underlying
system resource that performs asynchronous (or synchronous) operations. The
low-level Asio I/O objects must be associated with an `asio::io_context`.

The Asio I/O objects are only used within |amongoc|'s default event loop
implementation.


.. _asio.algo:

Asynchronous Algorithms
***********************

Asio's *asynchronous algorithms* are *extremely powerful* APIs that do not rely
on a particular event loop or any particular I/O object implementation. Instead,
the algorithms are written generically in terms of asynchronous object concepts,
such as `asio::AsyncReadStream` and `asio::AsyncWriteStream`. As long as the
operands to the algorithms meet the generic concept requirements, the algorithms
will Just Work™.

The Asio algorithms are used as the basis for the wire protocol implementation
in |amongoc|. The `amongoc::tcp_connection_rw_stream` type implements
`asio::AsyncReadStream` and `asio::AsyncWriteStream`, so it can be used for any
Asio algorithm that expects such an interface.

APIs of Note
############

Asio APIs
*********

.. class:: asio::io_context

  This is the default Asio event loop type. :ref:`I/O objects <asio.io-objs>`
  are created and associated with an `io_context`.

  .. seealso:: `The Asio documentation for io_context`__

  __ https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/io_context.html

  In |amongoc|, `io_context` is used as the backing implementation of the
  :func:`default event loop <amongoc_default_loop_init>`.

  At no other place in |amongoc| is `asio::io_context` used for any purpose.
  |amongoc| allows the user to provide their own event loop, so we cannot assume
  that we have an Asio `io_context` available.


.. concept:: template <typename T> asio::CompletionToken

  This is an exposition-only concept from Asio for CompletionToken_ objects.

  A CompletionToken is an object that tells an asynchronous operation API function
  two things:

  1. How to *initiate* an operation
  2. How to *complete* an operation

  Completion tokens are further subdivided by the type of completion signature
  they expect. For example, WriteToken_, ReadToken_, HandshakeToken_.

  In the simplest case, a `CompletionToken` is simply a callback function that
  accepts arguments for information about the completed operation. When such a
  callback is used as a token, the operation is initiated immediately and the
  function is called when it completes.

  `CompletionToken`\ s can also customize the initiation and return value of the
  initiating function in order to change the way the operation is initiated.
  Asio ships with several completion tokens that tweak the way a function is
  invoked. For example, `asio::deferred` is a completion token that causes the
  operation to return an *initiator* function, that will only start the
  operation when it is called.

  |amongoc| defines one custom `CompletionToken` of interest:
  `amongoc::asio_as_nanosender`

.. _CompletionToken: https://think-async.com/Asio/asio-1.30.2/doc/asio/overview/model/completion_tokens.html
.. _ReadToken: https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/ReadToken.html
.. _WriteToken: https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/WriteToken.html
.. _HandshakeToken: https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/HandshakeToken.html

.. var:: auto asio::deferred

  A `CompletionToken` that defers the initiation of an associated operation.
  `Refer to the Asio documentation.`__

  __ https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/deferred.html

.. concept:: template <typename T> asio::AsyncReadStream

  .. var::
    CompletionToken auto token
    amongoc::wire::mutable_buffer_sequence auto bufs
    T& stream

    .. rubric:: Requires

    - :expr:`stream.async_read_some(bufs, token)`

    Must read at least one byte of data from the underlying stream into the
    buffers `bufs`. When finished, must complete with
    :expr:`void(asio::error_code, std__size_t)`.

    .. note:: When we define `AsyncReadStream` types in |amongoc|, we assume
      that `token` is a regular completion handler: It does not use the full
      `CompletionToken` interface.

.. concept:: template <typename T> asio::AsyncWriteStream

  .. var::
    CompletionToken auto token
    amongoc::wire::const_buffer_sequence auto bufs
    T& stream

    .. rubric:: Requires

    - :expr:`stream.async_write_some(bufs, token)`

    Must write at least one byte of data into the underlying stream from the
    buffers `bufs`, and complete with
    :expr:`void(asio::error_code, std__size_t)`.


|amongoc| APIs for Asio
***********************

.. var:: auto amongoc::asio_as_nanosender

  This object implements the `asio::CompletionToken` interface and causes any
  Asio asynchronous operation APIs to return a `nanosender` instead of
  initiating the operation. This allows any Asio API to be used with the
  |amongoc| `nanosender`\ s transparently. The `sends_t` of the resulting
  nanosender is based on the completion signature of the associated operation
  via the following table:

  .. list-table::
    :header-rows: 1

    - - Asio completion signature
      - Resulting `sends_t`
    - - :cpp:`void()`
      - `mlib::unit`
    - - :cpp:`void(asio::error_code)`
      - `result\<mlib::unit, std::error_code>`
    - - :cpp:`void(asio::error_code, T)` (for some type |T|)
      - `result\<T, std::error_code>`

  Asio supports operations that have more than one completion signature, but
  `asio_as_nanosender` does not currently support this, and we do not use any
  Asio operations that do this.

  When `asio_as_nanosender` is used as an `asio::CompletionToken` for an
  operation, the return value from the Asio API will be an unspecified type that
  implements `nanosender`.


.. concept::
  template <typename T> amongoc::wire::const_buffer_sequence
  template <typename T> amongoc::wire::mutable_buffer_sequence

  These concepts correspond to Asio's ConstBufferSequence_ and
  MutableBufferSequence_ concepts.

.. _ConstBufferSequence: https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/ConstBufferSequence.html
.. _MutableBufferSequence: https://think-async.com/Asio/asio-1.30.2/doc/asio/reference/MutableBufferSequence.html


.. class::
  amongoc::tcp_connection_rw_stream

  This is a move-only class type that partially implements
  `asio::AsyncReadStream` and `asio::AsyncWriteStream`.

  .. member::
    amongoc_loop* loop
    unique_box conn

    `loop` is the event loop associated with the connection `conn`. `conn` is an
    opaque box that was obtained using `amongoc_loop_vtable::tcp_connect`.

  .. function::
    void async_read_some(wire::mutable_buffer_sequence auto&& bufs, auto&& cb)
    void async_write_some(wire::const_buffer_sequence auto&& bufs, auto&& cb)

    Implement the low-level read/write operations that Asio expects in order to
    construct :ref:`high-level I/O algorithms <asio.algo>`.

    The `cb` parameter must be a regular completion callback. The full
    `asio::CompletionToken` interface is not implemented.

    .. note::

      Don't use these directly. Instead, use the Asio
      :ref:`algorithms <asio.algo>`, which will allow you to use a proper
      `asio::CompletionToken` and have stronger behavioral guarantees.

    These member functions will invoke `amongoc_loop_vtable::tcp_read_some` and
    `amongoc_loop_vtable::tcp_write_some`, respectively.


.. rubric:: Footnotes

.. [#fn-sys-link]

  This difference is the primary reason that we use standalone Asio instead of
  Boost.Asio: Using :cpp:`boost::system::error_code` requires linking to the
  Boost.System library, whereas standalone Asio does not require any such
  linking.

  If, in the future, we otherwise would require linking to Boost.System for any
  other reason, it may be beneficial to switch to Boost.Asio simply to reduce
  the build complexity.
