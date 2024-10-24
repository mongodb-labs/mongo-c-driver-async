############
BSON Reading
############

.. header-file::
  bson/byte.h
  bson/view.h

  Contains APIs for reading BSON object data.


Types
#####

.. struct::
  [[zero_initializable]] bson_array_view
  [[zero_initializable]] bson_view

  These types represents null-able read-only view of a BSON document or array
  object. The object is pointer-sized, trivial, and zero-initializable. The
  `bson_array_view` has the same interface as `bson_view`, but disambiguates
  between arrays and documents at compile-time.

  :zero-initializable:
    |attr.zero-init| Represents a null view: A view that does not refer to any
    document, equivalent to `bson_view_null`. One can call `bson_data` to test
    for a null view. Using a null view is undefined behavior for most other
    operations.
  :header: :header-file:`bson/view.h`

  .. note:: This type should not be created manually. It should be created using
    `bson_as_view`, `bson_view_from_data`, `bson_iterator_document`, or
    :c:macro:`BSON_VIEW_NULL`.

  .. type::
    iterator = ::bson_iterator
    size_type = std__uint32_t

  .. function::
    static bson_view from_data(const bson_byte* b, size_t datalen)

    :C API: `bson_view_from_data`
    :return: The view returned by this function is guaranteed to be non-null.
    :throws: This function will throw an exception if it encounters a decoding
      error. For a non-throwing equivalent, use `bson_view_from_data`.

  .. function::
      iterator begin() const
      iterator end() const

      Access the BSON view as a range of elements

      :C API: `bson_begin` and `bson_end`

  .. function::
    explicit operator bool() const
    bool has_value() const

    Determine whether the given view is null.

  .. function:: const bson_byte* data() const

    Obtain a pointer to the document data referred-to by this view

  .. function:: bool empty() const

    Determine whether the given document object is empty

  .. function:: iterator find(std::string_view) const

    Return an iterator referring to the

  .. function::
    size_type byte_size() const

    :C API: `bson_size`


.. enum:: bson_view_errc

  Error conditions that can occur when attempting to decode a BSON document into
  a `bson_view`.

  :header: :header-file:`bson/view.h`

  .. enumerator:: bson_view_errc_okay

    There was no decoding error

  .. enumerator:: bson_view_errc_short_read

    The data buffer given for decoding is too short to possibly hold the
    document.

    If the data buffer is less that five bytes, it is impossible to be a valid
    document and this error will occur. If the buffer is more than five bytes,
    but the header declares a length that is greater than the buffer length,
    this error will also occur.

  .. enumerator:: bson_view_errc_invalid_header

    The BSON document header declares an invalid length.

    This will occur if the BSON header size is a negative value when it is
    decoded.

  .. enumerator:: bson_view_errc_invalid_terminator

    Decoding a document expects to find a nul byte at the end of the document
    data. This error will arise if that null byte is missing.


.. struct:: bson_byte

  A byte-sized plain data type that is used to encapsulate a byte value.

  :header: :header-file:`bson/byte.h`

  .. member:: uint8_t v

    The value of the octect represented by this byte

  .. function::
    constexpr explicit operator std__byte() const noexcept
    constexpr explicit operator std__uint8_t() const noexcept
    constexpr explicit operator char() const noexcept

    (C++) explicit conversion operators for BSON byte values.


.. type::
  __bson_viewable

  A parameter annotated as `__bson_viewable` accepts any type that can be viewed
  with the BSON iteration APIs. This includes:

  - `bson_view`
  - `bson_doc`
  - `bson_array_view`
  - `bson_mut`


Functions & Macros
##################

View Inspection
***************

.. function::
  bson_view bson_as_view(__bson_viewable [[nullable]] B)

  Obtain a `bson_view` for the given document-like object. This is also used by
  other function-like macros to coerce `bson_mut` and `bson_doc` to `bson_view`
  automatically.

  :param B: A `bson_mut`, `bson_doc`, or `bson_view`.
  :return: A new `bson_view` that views the document associated with ``B``.

  .. note:: |macro-impl|


.. function:: bson_view bson_view_from_data(const bson_byte* const data, const size_t data_len, enum bson_view_errc* error)

  Obtain a new `bson_view` that views a document that exists at `data` which is
  *at most* `data_len` bytes long.

  :param data: A pointer to the beginning of a BSON document. This sould point
    exactly at the BSON object header.
  :param data_len: The length of the array of bytes pointed-to by `data`. This
    function will validate the document header to ensure that it will not
    attempt to overrun the `data` buffer.
  :param error: An optional output parameter that will describe the error encountered
    while decoding a BSON document from `data`.
  :return: A `bson_view` that views the document at `data`, or a null view if an
    error occured. Checking for null can be done with `bson_data`.
  :header: :header-file:`bson/view.h`

  The returned view is valid until:

  - Dereferencing `data` would be undefined behavior, including if the
    underlying buffer is reallocated during mutation.
  - OR any data accessible via `data` is modified outside of a BSON mutator API.

  .. important::

    This function *does not* consider it an error if `data_len` is larger than
    the actual document size. This is a useful behavior for decoding data from
    an input stream.

    The actual resulting document size can be obtained with `bson_size`

  .. important::

    This function *does not* validate the content of elements within the
    document. The document elements are validated on-the-fly during iteration.
    Refer: :ref:`bson.iter.errant`


.. function::
  const bson_byte* bson_data(__bson_viewable [[nullable]] B)
  bson_byte* bson_mut_data(__bson_viewable [[nullable]] B)

  Obtain a pointer to `bson_byte` referring to the first byte in the given
  document. |attr.nullable| If the given object is a null view/document, this
  returns a null pointer.

  :header: :header-file:`bson/iterator.h`

  The argument to `bson_mut_data` cannot be a `bson_view`, as that is read-only.
  This will evaluate to a null pointer if ``B`` is a null view/document object.

  .. note:: |macro-impl|


.. function::
  uint32_t bson_size(__bson_viewable B)
  int32_t bson_ssize(__bson_viewable B)

  Obtain the size of the given document object, in bytes.

  :C++ API: `bson_view::byte_size`
  :param B: A `bson_view`, `bson_doc`, or `bson_mut`.
  :return: `bson_size` returns a `uint32_t`, while `bson_ssize` returns an
    `int32_t`
  :header: :header-file:`bson/iterator.h`

  .. note:: |macro-impl|
