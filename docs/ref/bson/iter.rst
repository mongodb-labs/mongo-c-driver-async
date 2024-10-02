##############
BSON Iteration
##############

.. header-file:: bson/iterator.h
.. |this-header| replace:: :header-file:`bson/iterator.h`

.. note::

  Using `bson_iterator::reference` in C++ and certain document decoding C APIs
  requires the inclusion of the :header-file:`bson/view.h`.

Types
#####

.. struct:: bson_iterator

  A BSON iterator is used to refer to elements and positions within a BSON
  document. It can be used to read elements from a `bson_view` or `bson_doc`,
  and is also used when modifying elements with `bson_mut`.

  :header: |this-header|

  This type is fully trivial and is the size of two pointers.

  This struct also implements the C++ `std__forward_iterator` concept.

  .. function::
    reference operator*() const
    auto operator->() const

    Obtain a `reference` to the element. This will :cpp:`throw` an exception if
    the iterator is :ref:`errant <bson.iter.errant>`.

  .. function::
    bson_iterator& operator++()
    bool operator==(const bson_iterator) const

    Implements `std__forward_iterator`.

    :C API: `bson_next` and `bson_iterator_eq`

  .. function::
    bson_iter_errc error() const
    bool has_error() const

    :C API: `bson_iterator_get_error`

  .. function::
    bool has_value() const

    :return: :expr:`not stop()`

  .. function::
    bool stop() const

    :C API: `bson_stop`

  .. function::
    const bson_byte* data() const
    size_type data_size() const

    :C API: `bson_iterator_data` and `bson_iterator_data_size`

  .. function::
    void throw_if_error() const

    If :expr:`has_error()`, throws an exception related to that error.

.. class:: bson_iterator::reference

    (C++) Implements a proxy reference for the iterator for using with
    `operator*` and `operator->`. This class is used to access properties of the
    underlying element.

    :header: :header-file:`bson/view.h`

    .. function::
      bson_type type() const
      std::string_view key() const

      :C API: `bson_key` and `bson_iterator_type`

    .. _iter.reference.access:

    .. function::
      double double_() const
      std__string_view utf8() const
      bson_view document() const
      bson_view document(bson_view_errc& doc_errc) const
      bson_binary binary() const
      bson_oid oid() const
      bool bool_() const
      bson_datetime datetime() const
      bson_regex regex() const
      bson_dbpointer dbpointer() const
      bson_code code() const
      bson_symbol symbol() const
      std__int32_t int32() const
      bson_timestamp timestamp() const
      std__int64_t int64() const
      bson_decimal128 decimal128() const

      Obtain the referred-to element value.

      :C API: Use the :ref:`C API iterator access methods <iter.access>`
      :throw: None of these functions throw any error. If the iterator has the
        incorrect type, then a zero-initialized value of the corresponding type
        will be returned instead.

    .. _iter.reference.coerce:

    .. function::
      double as_double(bool* okay = nullptr) const
      bool as_bool() const
      std__int32_t as_int32(bool* okay = nullptr) const
      std__int64_t as_int64(bool* okay = nullptr) const

      :C API: :ref:`C API iterator coersion functions <iter.coerce>`

    .. function::
      template <typename F> \
      decltype(auto) visit(F&& fn) const

      Apply the value-visitor `fn` to the underlying element value.

      :param fn: An invocable object. Must be invocable with each data type
        that an element can hold. Each overload of the invocation must return
        the same type.
      :return: Returns the value obtained by invoking the visior function with
        the appropriate value.

    .. function::
      template <typename T> \
      std::optional<T> try_as() const

      Attempt to obtain a value from the element with the given type. The type
      must correspond to one of the data types returned by
      :ref:`the accessor methods <iter.reference.access>`.


.. enum:: bson_type

  This enumeration corresponds to the types of BSON elements. Their numeric
  value is equal to the octet value that is encoded in the BSON data itself.

  .. enumerator::
    bson_type_eod = 0x00
    bson_type_double = 0x01
    bson_type_utf8 = 0x02
    bson_type_document = 0x03
    bson_type_array = 0x04
    bson_type_binary = 0x05
    bson_type_undefined = 0x06
    bson_type_oid = 0x07
    bson_type_bool = 0x08
    bson_type_date_time = 0x09
    bson_type_null = 0x0A
    bson_type_regex = 0x0B
    bson_type_dbpointer = 0x0C
    bson_type_code = 0x0D
    bson_type_symbol = 0x0E
    bson_type_codewscope = 0x0F
    bson_type_int32 = 0x10
    bson_type_timestamp = 0x11
    bson_type_int64 = 0x12
    bson_type_decimal128 = 0x13
    bson_type_maxkey = 0x7F
    bson_type_minkey = 0xFF

.. enum:: bson_iter_errc

  Error conditions that may occur during BSON iteration. See: :ref:`bson.iter.errant`

  .. enumerator:: bson_iter_errc_okay

    No error condition.

  .. enumerator:: bson_iter_errc_short_read

    The document ended abruptly before being able to read another element.

  .. enumerator:: bson_iter_errc_invalid_type

    An invalid type tag was encountered, and iteration cannot decode the value.

  .. enumerator:: bson_iter_errc_invalid_length

    An element declares itself to have a size that is too large to fit within
    the document which contains it.


Functions & Macros
##################

Document Iteration
******************

.. function::
  bson_iterator bson_begin(auto B)
  bson_iterator bson_end(auto B)

  Obtain a `bson_iterator` referring to the beginning or end of the given BSON
  document, respectively.

  :C++ API: Use the ``begin()`` and ``end()`` member function of the object `B`
  :param B: A BSON document to be viewed. Passed through :c:macro:`bson_as_view`.
  :return: A `bson_iterator` referring to the respective positions.

  .. important::

    `bson_begin` may return an :ref:`errant iterator <bson.iter.errant>` if
    decoding the first element fails.

  .. note:: |macro-impl|


.. function::
  bson_iterator bson_next(bson_iterator i)

  Obtain a `bson_iterator` referring to the next element after `i`, or an
  :ref:`errant iterator <bson.iter.errant>` if a parsing error occurs.

  :C++ API: `bson_iterator::operator++`
  :param i: An iterator referring to some document. The iterator must refer to
    a valid element.
  :precondition: :expr:`not bson_stop(i)`


.. function::
  bool bson_iterator_eq(bson_iterator a, bson_iterator b)

  Determine whether the iterators `a` and `b` refer to the same element within
  their respective document.

  :C++ API: `bson_iterator::operator==`


.. function::
  bool bson_stop(bson_iterator it)

  Determine whether the given iterator can be advanced further.

  This function will return :cpp:`true` if `it` is the end iterator *or* if `it`
  has :ref:`encountered a decoding error <bson.iter.errant>` while it was
  advanced.


.. function::
  bson_iter_errc bson_iterator_get_error(bson_iterator it)

  Obtain the error condition for the given iterator. If the iterator is valid,
  returns `bson_iter_errc::bson_iter_errc_okay`. See: :ref:`bson.iter.errant`


.. function:: bson_iterator bson_find(auto B, auto Key)

  Obtain a `bson_iterator` referring to the first element within ``B`` that has
  the key ``Key``

  :param B: A BSON document object, passed through `bson_as_view`.
  :param Key: A key to search for. Passed through `bson_as_utf8`.
  :return: A `bson_iterator`. If the expected key was found, returns an
    iterator referring to that element.

    If an error occured during iteration, the returned iterator will have an
    associated error (see: `bson_iterator_get_error`).

    If the requested element was not found, returns :cpp:`bson_end(B)`

  .. note:: |macro-impl|


Looping
=======

.. c:macro::
  bson_foreach(IterName, Viewable)
  bson_foreach_subrange(IterName, FirstIter, LastIter)

  These macros allow the creation of control flow loops that iterate over the
  elements of a BSON document or array.

  :param IterName: An identifier that will be the name of the `bson_iterator`
    that will be in scope for the loop body.
  :param Viewable: A BSON document object, passed through :c:macro:`bson_as_view`
  :param FirstIter: A first iterator to begin iteration.
  :param LastIter: The iterator at which to stop the loop.

  ::

    bson_foreach(it, my_doc) {
      // loop body
    }

  For every element in the document/range, an iterator (named by ``IterName``)
  will point to that element. The created iterator is :cpp:`const`-qualified.
  If the document being inspected is modified during execution of the loop,
  the behavior is undefined.

  .. rubric:: Error Behavior

  If a call to `bson_next` results in an
  :ref:`errant iterator <bson.iter.errant>`, then the loop will be executed
  *once* using that errant iterator, and then the loop will stop on the next
  iteration. For this reason, it is important to check that the iterator is not
  errant (see `bson_iterator_get_error`).


Element Properties
******************

.. function::
  bson_utf8_view bson_key(bson_iterator it)

  Obtain a string that views the key of the element referred-to by `it`

  :precondition: :expr:`not bson_stop(it)`.


.. function::
  bool bson_key_eq(bson_iterator it, auto K)

  Test whether the `it` element key is equal to the given string.

  :param it: The element to inspect.
  :param K: A string to compare against. Passed through `bson_as_utf8`

  .. note:: |macro-impl|

.. function::
  bson_type bson_iterator_type(bson_iterator it)

  Get the type of the element referred-to by `it`

  :precondition: :expr:`not bson_iterator_get_error(it)`


.. function::
  const bson_byte* bson_iterator_data(bson_iterator it)

  Obtain a pointer to the element data referred-to by `it`.

  :precondition: :expr:`not bson_iterator_get_error(it)`


.. function::
  uint32_t bson_iterator_data_size(bson_iterator it)

  Obtain the size of the element referred-to by `it`, as a number of bytes.


.. _iter.access:

Element Value Access
********************

.. function::
  double bson_iterator_double(bson_iterator it)
  bson_utf8_view bson_iterator_utf8(bson_iterator it)
  bson_view bson_iterator_document(bson_iterator it, bson_view_errc* doc_errc)
  bson_binary bson_iterator_binary(bson_iterator it)
  bson_oid bson_iterator_oid(bson_iterator it)
  bool bson_iterator_bool(bson_iterator it)
  bson_datetime bson_iterator_datetime(bson_iterator it)
  bson_regex bson_iterator_regex(bson_iterator it)
  bson_dbpointer bson_iterator_dbpointer(bson_iterator it)
  bson_code bson_iterator_code(bson_iterator it)
  bson_symbol bson_iterator_symbol(bson_iterator it)
  int32_t bson_iterator_int32(bson_iterator it)
  bson_timestamp bson_iterator_timestamp(bson_iterator it)
  int64_t bson_iterator_int64(bson_iterator it)
  bson_decimal128 bson_iterator_decimal128(bson_iterator it)

  Obtain the decoded value of the referred-to element for the given iterator.

  :C++ API: Use the :ref:`reference access methods <iter.reference.access>`
  :param it: The iterator referred to a valid element.
  :param doc_errc: For `bson_iterator_document`, an optional output parameter
    that is written in case that the decoded document is invalid. If such an
    error occurs, then the returned `bson_view` will be null. Upon success, the
    value `bson_view_errc_okay <bson_view_errc::bson_view_errc_okay>` is
    written.

  If the `bson_iterator_type` of the iterator `it` does not match the target
  type, returns a zero-initialized value of the corresponding type.

.. _iter.coerce:
.. function::
  double bson_iterator_as_double(bson_iterator it, bool* okay)
  bool bson_iterator_as_bool(bson_iterator it)
  int32_t bson_iterator_as_int32(bson_iterator it, bool* okay)
  int64_t bson_iterator_as_int64(bson_iterator it, bool* okay)

  Coerce the given BSON value to an numeric type.

  :C++ API: :ref:`The reference proxy coersion functions <iter.reference.coerce>`
  :param it: A valid iterator to be decoded.
  :param okay: An optional output parameter. If the returned value corresponds
    to the decoded element value, this is set to :cpp:`true`. Otherwise, this
    will be set to :cpp:`false`. `bson_iterator_as_bool` always coerces, so it
    omits this parameter.

  .. rubric:: Notes on behavior:

  - For coercing a boolean element to a number, a :cpp:`true` element becomes :cpp:`1`,
    and :cpp:`false` becomes :cpp:`0`.
  - Coercing between double, int32, and int64 will do the coercing according to
    the language's conversion rules. (e.g. an **int64** converted to a
    **double** will first extract an `int64_t` and then convert it to a
    :cpp:`double`)
  - For coercing to a ``bool`` with `bson_iterator_as_bool`, the following
    applies:

    - If the element is a numeric type, a :cpp:`true` value will be returned if
      if is not equal to :cpp:`0`. Otherwise, it returns :cpp:`false`.
    - For documents, arrays, and strings, the returned value is a bool
      corresponding to whether the corresponding value is non-empty. If decoding
      a subdocument fails, returns :cpp:`false`.
    - For BSON types **EOD**, **undefined**, **null**, **min-key**, and
      **max-key**, returns :cpp:`false`.
    - For all other BSON types, returns :cpp:`true` unconditionally.


Behavioral Notes
################

.. _bson.iter.errant:

Errant Iterators
****************

BSON documents are validated on-the-fly as the iterator is advanced. If a parse
error occurs during `bson_next` or `bson_begin`, then an *errant iterator* will
be created. Attempting to read from or advance an errant iterator will result in
undefined behavior. In C++, the `bson_iterator::operator*` and
`bson_iterator::operator->` will throw an exception if the iterator is errant.

To test whether an iterator has an error, use `bson_iterator_get_error` (C) or
`bson_iterator::error()` (C++).
