############
Status Codes
############

.. header-file:: amongoc/status.h

  Types and functions for handling generic status codes


Types
#####

.. struct:: amongoc_status

  An augmented status code object.

  .. note:: This API is inspired by the C++ `std__error_code` API

  :zero-initialized: A |zero-initialized| `amongoc_status` has **no defined
    semantics**. If you want a default "okay" status, use `amongoc_okay`. To
    create a constant-initialized okay status, use :cpp:`{&amongoc_generic_category, 0}`
    as an initializer.

  .. member:: const amongoc_status_category_vtable* category

    Pointer to a virtual method table for status categories. This pointer can
    be compared against the address of known categories to check the category
    of the status.

    .. seealso:: :ref:`amongoc-status-categories`

  .. member:: int code

    The actual status code. The semantics of this value depend on the
    `category` of the error.

    .. important::

      DO NOT assume that a non-zero value represents an error, as the status
      category may define more than one type of success value. Use
      `amongoc_is_error` to check whether a status represents an error.

  .. function::
    static amongoc_status from(std__error_code) noexcept
    static amongoc_status from(std__errc) noexcept
    std__error_code as_error_code() const noexcept

    |C++ API| Construct a new `amongoc_status` from a `std__error_code`, or
    convert an existing `amongoc_status` to a C++ `std__error_code`

    .. warning::

      This conversion is *potentially lossy*, as the `category` of the
      status/error code might not have an equivalent category in the target
      representation. This method should only be used on status values that are
      know to have a category belonging to |amongoc| (See:
      :ref:`amongoc-status-categories`).

      It is recommended to keep to using `amongoc_status` whenever possible.

  .. function:: std__string message() const noexcept

    :C API: `amongoc_message`

  .. function:: bool is_error() const noexcept

    :C API: `amongoc_is_error`


Functions & Macros
##################

.. function:: bool amongoc_is_error(amongoc_status st)

  Test whether the given status `st` represents an error condition.

  :param st: The statust to be checked
  :return: ``true`` if `st` represents an error.
  :return: ``false`` otherwise.

  .. note::

    The defintion of what constitutes an error depends on the
    `amongoc_status::category`.

  .. seealso:: :c:macro:`amongoc_if_error`


.. function:: bool amongoc_is_cancellation(amongoc_status st)

  Test whether the given status `st` represents a cancellation.

  :param st: The status to be checked.
  :return: ``true`` if `st` represents a cancellation.
  :return: ``false`` otherwise.

  .. note::

    The defintion of what constitutes a cancellation depends on the
    `amongoc_status::category`.


.. function:: bool amongoc_is_timeout(amongoc_status st)

  Test whether the given status `st` represents an operation timeout.

  :param st: The status to be checked.
  :return: ``true`` if `st` represents a timeout.
  :return: ``false`` otherwise.

  .. note::

    The defintion of what constitutes a timeout depends on the
    `amongoc_status::category`.


.. function:: const char* amongoc_message(amongoc_status st, char* buf, size_t buflen)

  Obtain a human-readable message describing the status `st`.

  :param st: The status to inspect.
  :param buf: Pointer to a modifiable |char| array of at least `buflen` |char|\ s.
    This argument may be a null pointer if `buflen` is zero.
  :param buflen: The length of the |char| array pointed-to by `buf`, or zero
    if `buf` is a null pointer.
  :return: A non-null pointer |S| to a :term:`C string`.

  The buffer `buf` *may* be used by this function as storage for a
  dynamically-generated message string, but the function is not required to
  modify `buf`. The returned pointer |S| is never null, and may or may not be
  equal to `buf`.

  This function does not dynamically allocate any memory.

  .. seealso::

    - :c:macro:`amongoc_declmsg` for concisely obtaining the message from a
      status object.
    - :c:macro:`amongoc_if_error` to check for an error and extract the message
      in a single line.


.. c:macro:: amongoc_declmsg(MsgVar, Status)

  This statement-like macro will obtain the status message :term:`C string` from
  the given status ``Status`` and place it in a variable identified by
  ``MsgVar``.

  :param MsgVar: Must be an identifier. This macro will declare a variable of
    type ``const char*`` with this name, which will contain the message from
    ``Status``.
  :param Status: Any expression of type `amongoc_status`.

  This macro is a shorthand for the following::

    char __buffer[128];
    const char* MsgVar = amongoc_message(Status, __buffer, sizeof __buffer)


.. c:macro::
  amongoc_if_error(Status, MsgVar, StatusVar)

  Create a branch on whether the given status represents an error. This macro
  supports being called with two arguments, or with three::

    amongoc_if_error (status, msg_varname) {
      fprintf(stderr, "Error message: %s\n", msg_varname)
    }

  ::

    amongoc_if_error (status, msg_varname, status_varname) {
      fprintf(stderr, "Error code %d has message: %s\n", status_varname.code, msg_varname);
    }

  :param Status: The first argument must be an expression of type `amongoc_status`. This is
    the status to be inspected.
  :param MsgVar: This argument should be a plain identifier, which will be declared within
    the scope of the statement as the :term:`C string` for the status.
  :param StatusVar: If provided, a variable of type `amongoc_status` will be declared within
    the statement scope that captures the value of the ``Status`` argument.

  .. hint::

    If you are using ``clang-format``, add ``amongoc_if_error`` to the
    ``IfMacros`` for your ``clang-format`` configuration.

.. var:: const amongoc_status amongoc_okay

  A generic status with a code zero. This represents a generic non-error status.

  .. note:: |macro-impl|.


Status Categories
#################

.. struct:: amongoc_status_category_vtable

  A virtual-method table for `amongoc_status` that defines the semantics of
  status codes. The following "methods" are actually function pointers that
  may be customized by the user to provide new status code behaviors.

  .. |the-code| replace:: The integer status code from `amongoc_status::code`

  .. rubric:: Customization Points

  .. function:: const char* name()

    :return: Must return a statically-allocated null-terminated string that
      uniquely identifies the category.

  .. function:: const char* message(int code, char* buf, size_t buflen)

    .. seealso:: User code should use `amongoc_message` instead of calling this function directly.

    :param code: |the-code|
    :param buf: Pointer to an array of |char| at least `buflen` long. This may be null
      if `buflen` is zero.
    :param buflen: The length of the character array pointed-to by `buf`. If this
      is zero, then `buf` may be a null pointer.
    :return: Should return a pointer to a :term:`C string` that provides a
      human-readable message describing the status code `code`. May return a null
      pointer if there is a failure to generate the message text.

    A valid implementation of `message` should do the following:

    1. If the message for `code` is a statically allocated :term:`C string` |S|,
       return |S| without inspecting `buf`.
    2. If the message |M| needs to be dynamically generated and `buf` is not
       null, generate the message string in `buf`, ensuring that `buf` contains
       a nul terminator. The written length with nul terminator must not exceed :expr:`buflen` (use of ``snprintf`` is
       recommended). Return `buf`.
    3. Otherwise, return a fallback message string or a null pointer.

    If this function returns a null pointer, then `amongoc_message` will replace
    it with a fallback message telling the caller that the message text is
    unavailable.

  .. function:: bool is_error(int code) [[optional]]

    :param code: |the-code|
    :return:
      Should return ``true`` if-and-only-if the integer value of `code` represents
      a non-success state (this includes cancellation and timeout).

    .. note:: If this function is not defined, `amongoc_is_error` returns ``true``
        if `code` is non-zero

  .. function:: bool is_cancellation(int code) [[optional]]

    :param code: |the-code|
    :return: Should return ``true`` if the value of `code` represents a cancellation
      (e.g. POSIX ``ECANCELLED``).

    .. note:: If this function is not defined, `amongoc_is_cancellation` will always
      return ``false``.

  .. function:: bool is_timeout(int code) [[optional]]

    :param code: |the-code|
    :return: Should return ``true`` if the value of `code` represents a timeout
      (e.g. POSIX ``ETIMEDOUT``).

    .. note:: If this function is not defined, `amongoc_is_timeout` will always
      return ``false``.


.. _amongoc-status-categories:

Built-In |amongoc| Categories
*****************************

.. var::
    const amongoc_status_category_vtable amongoc_generic_category
    const amongoc_status_category_vtable amongoc_system_category
    const amongoc_status_category_vtable amongoc_netdb_category
    const amongoc_status_category_vtable amongoc_addrinfo_category
    const amongoc_status_category_vtable amongoc_io_category
    const amongoc_status_category_vtable amongoc_server_category
    const amongoc_status_category_vtable amongoc_client_category
    const amongoc_status_category_vtable amongoc_tls_category
    const amongoc_status_category_vtable amongoc_unknown_category

  The above `amongoc_status_category_vtable` objects are the built-in status
  categories provided by |amongoc|. Each has the following meaning:

  .. index:: pair: status category; amongoc.generic

  *generic* (``amongoc.generic``)
    Corresponds to POSIX ``errno`` values. With this category, `amongoc_status::code`
    corresponds to a possible error code macro from ``<errno.h>``

  .. index:: pair: status category; amongoc.system

  *system* (``amongoc.system``)
    Corresponds to error code values dependent on the host platform. On Unix-like
    systems, these error code values will be equivalent to those of `amongoc_generic_category`.

    On Windows, for example, the `amongoc_status::code` will be a value obtained
    from `GetLastError()`__

    __ https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror

  .. index:: pair: status category; amongoc.addrinfo
  .. index:: pair: status category; amongoc.netdb

  *addrinfo* (``amongoc.addrinfo``) & *netdb* (``amongoc.netdb``)
    Error codes related to name resolution and network addressing. The error code
    values depend on the error codes exposed by the host's networking system.

    These statuses get their own category separate from *system* and *generic*
    because most platforms' networking implementations reuse POSIX integer
    values for error codes that arise from name resolution, thus it is required
    that such errors are distinguished by their category to avoid ambiguity.

  .. index:: pair: status category; amongoc.io

  *io*
    Error codes related to I/O that are not covered in the system or generic
    category.

  .. index:: pair: status category; amongoc.server

  *server* (``amongoc.server``)
    These error conditions correspond to error codes returned from a MongoDB
    server. These values are named in :enum:`amongoc_server_errc`.

  .. index:: ! pair: status category; amongoc.client

  *client* (``amongoc.client``)
    These error conditions correspond to erroneous use of client-side APIs.
    These arise to prevent communication with a server in a way that would
    likely cause undesired behavior, often from client/server incompatibilities.
    These error values are named in :enum:`amongoc_client_errc`.

  .. index:: pair: status category; amongoc.tls

  *tls* (``amongoc.tls``)
    Error conditions related to TLS. Often the corresponding integer value comes
    from OpenSSL. Error reason values are stored in `amongoc_tls_errc`

  .. index:: pair: status category; amongoc.unknown

  *unknown* (``amongoc.unknown``)
    This status category appears if the status was constructed from an unknown
    source. In this case, no status messages or status semantics are defined, except
    that `amongoc_is_error` returns ``false`` only if the `amongoc_status::code` is ``0``.

    The message returned from `amongoc_message` will always be
    "``amongoc.unknown:<n>``" where ``<n>`` is the numeric value of the error
    code.


Status Code Enumerations
========================

.. index:: ! pair: status codes; amongoc.server
.. enum:: amongoc_server_errc

  This enum contains error code values corresponding to their numeric value
  as returned from a MongoDB server.

  .. seealso::

    `The MongoDB Error Codes Reference`__

    __ https://www.mongodb.com/docs/manual/reference/error-codes/

  .. note:: This enum is not exhaustive, and it is possible for a server to
    return an error code that does not have a corresponding enumerator.

.. index:: ! pair: status codes; amongoc.client
.. enum:: amongoc_client_errc

  This enum corresponds to error codes that may arise for the
  `amongoc_client_category` status category.

  .. enumerator:: amongoc_client_errc_okay = 0

    Represents no error

  .. enumerator:: amongoc_client_errc_invalid_update_document

    Issued during update CRUD operations where the update specification document
    is invalid.


.. index:: ! pair: status codes; amongoc.tls
.. enum:: amongoc_tls_errc

  This enum corresponds to reason error codes related to TLS.

  .. important::

    Note that the `amongoc_status::code` value will not necessarily directly
    compare equal to any enumerator value in this enum. Instead, the reason
    should be extracted using `amongoc_status_tls_reason`, which extracts the
    reason portion of the status code from the status.

  Enumerators with an ``_ossl_`` in their identifier correspond to the OpenSSL
  error reasons from ``<openssl/sslerr.h>``.

  .. enumerator::
    amongoc_tls_errc_okay = 0

    This represents a non-error condition.

    There are many additional enumerators for this category, but they are not
    listed here. Most enumerators correspond to OpenSSL reason codes.


.. function:: amongoc_tls_errc amongoc_status_tls_reason(amongoc_status st)

  Extract the TLS reason integer value from a status code.

  If `st` does not have the `amongoc_tls_category` category, this will return
  `amongoc_tls_errc::amongoc_tls_errc_okay` (non-error). Otherwise, it will
  return a non-zero `amongoc_tls_errc` that specifies the error reason.


C++ Exception Type
##################

.. class:: amongoc::exception : public std::runtime_error

  A C++ exception type that carries an `amongoc_status` value.

  .. note:: This type is not currently thrown by any public APIs and is only used internally

  .. function:: exception(amongoc_status)

    Construct an exception object with the associated status.

  .. function:: amongoc_status status() const noexcept

    Return the `amongoc_status` associated with this exception.
