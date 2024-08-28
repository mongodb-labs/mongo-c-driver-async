Header: ``amongoc/status.h``
############################

.. header-file:: amongoc/status.h

  Types and functions for handling generic status codes

.. struct:: amongoc_status

  An augmented status code object.

  .. note:: This API is inspired by the C++ ``std::error_code`` API

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
    static amongoc_status from(std::error_code) noexcept
    std::error_code as_error_code() const noexcept

    |C++ API| Construct a new `amongoc_status` from a `std::error_code`, or
    convert an existing `amongoc_status` to a C++ `std::error_code`

    .. warning::

      This conversion is *potentially lossy*, as the `category` of the
      status/error code might not have an equivalent category in the target
      representation. This method should only be used on status values that are
      know to have a category belonging to |amongoc| (See:
      :ref:`amongoc-status-categories`).

      It is recommended to keep to using `amongoc_status` whenever possible.

  .. function:: std::string message() const noexcept

    :C API: `amongoc_status_strdup_message`

  .. function:: bool is_error() const noexcept

    :C API: `amongoc_is_error`

.. function:: bool amongoc_is_error(amongoc_status st)

  Test whether the given status `st` represents an error condition.

  :param st: The statust to be checked
  :return: ``true`` if `st` represents an error.
  :return: ``false`` otherwise.

  .. note::

    The defintion of what constitutes an error depends on the
    `amongoc_status::category`.

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


.. function:: char* amongoc_status_strdup_message(amongoc_status)

  Obtain a *dynamically allocated* C string that describes the status in
  human-readable form.

  .. important:: The returned string must be freed with ``free()``

  :C++ API: `amongoc_status::message`


.. var:: amongoc_status amongoc_okay

  A generic status with a code zero. This represents a generic non-error status.

  .. note:: This is implemented as a macro for C compatibility, and is therefore an r-value expression.

C++ APIs
********

.. rubric:: Namespace: ``amongoc``
.. namespace:: amongoc
.. type:: status = ::amongoc_status

  Alias of `::amongoc_status`

.. namespace:: 0


Status Categories
*****************

.. struct:: amongoc_status_category_vtable

  A virtual-method table for `amongoc_status` that defines the semantics of
  status codes. The following "methods" are actually function pointers that
  may be customized by the user to provide new status code behaviors.

  .. rubric:: Customization Points

  .. function:: const char* name()

    :return: Must return a statically-allocated null-terminated string that
      uniquely identifies the category.

  .. function:: char* strdup_message(int code)

    .. |the-code| replace:: The integer status code from `amongoc_status::code`

    :param code: |the-code|
    :return: Must return a dynamically allocated null-terminated string that
      describes the status in a human-readable format. The returned string will
      be freed with ``free()``.

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

  .. index:: pair: status category; amongoc.unknown

  *unknown* (``amongoc.unknown``)
    This status category appears if the status was constructed from an unknown
    source. In this case, no status messages or status semantics are defined, except
    that `amongoc_is_error` returns ``false`` only if the `amongoc_status::code` is ``0``.

    The message returned from `amongoc_status_strdup_message` will always be
    "``amongoc.unknown:<n>``" where ``<n>`` is the numeric value of the error
    code.


C++ Exception Type
##################

.. namespace:: amongoc
.. class:: exception : public std::runtime_error

  A C++ exception type that carries an `amongoc_status` value.

  .. note:: This type is not currently thrown by any public APIs and is only used internally

  .. function:: exception(amongoc_status)

    Construct an exception object with the associated status.

  .. function:: amongoc_status status() const noexcept

    Return the `amongoc_status` associated with this exception.
