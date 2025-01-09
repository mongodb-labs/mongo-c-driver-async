###########
BSON Values
###########

.. header-file::
    bson/value_ref.h
    bson/value.h

    Contains APIs for working with arbitrary BSON element values.


Types
#####

.. struct:: [[zero_initializable]] bson_value_ref

  A non-owning read-only view of a BSON value.

  .. _bson-null-ref:

  :zero-initialized: |attr.zero-init| Has type `bson_type_eod`, and represents a
    *null* value reference. **Note** that this is distinct from a reference to a
    `bson_type_null` value!

  .. note:: This is a union-like type. Only one of the value fields is active
    at a time. Check `type` before accessing other fields.

  .. note::

    For simple built-in types and aggregate types, the value ref takes a
    by-value copy of the value. It does not store a pointer to simple types.
    The "reference" semantics apply to the non-trivial types such as strings
    and document references.

  Members
  *******

  .. member:: bson_type type

    The active type of the viewed value. The special type tag `bson_type_eod`
    represents a null value reference.

  .. union:: __anon

    .. note:: The following are accessed directly on the `bson_value_ref` instance

    .. member::
      bson_eod eod
      double double_
      mlib_str_view utf8
      bson_view document
      bson_array_view array
      bson_binary_view binary
      bson_undefined undefined
      bson_oid oid
      bool bool_
      bson_datetime datetime
      bson_null null
      bson_regex_view regex
      bson_dbpointer_view dbpointer
      bson_code_view code
      bson_symbol_view symbol
      int32_t int32
      bson_timestamp timestamp
      int64_t int64
      bson_decimal128 decimal128
      bson_maxkey maxkey
      bson_minkey minkey

      The various values that can be viewed by the reference. Each field
      corresponds to a value of the `type` member.

  C++ Members
  ***********

  .. function::
    static bson_value_ref from(__bson_value_convertible V)

    Obtain a `bson_value_ref` instance that views the given value `V`

  .. function::
    bool as_bool()
    std__int32_t as_int32(bool* okay = nullptr)
    std__int64_t as_int64(bool* okay = nullptr)
    double as_double(bool* okay = nullptr)

    :C API: `bson_value_as_bool`, `bson_value_as_int32`, `bson_value_as_int64`, and `bson_value_as_double`

  .. function::
    bson_view get_document_or_array(bson_view dflt = {})

    If the value is a document or an array, return a view of that value.
    Otherwise, returns `dflt`.

  .. function::
    bool operator==(std::integral auto i)
    bool operator==(std__string_view sv)

    Compare the value to an integer or a string. If the value does not carry
    the appropriate type, returns |false|.

  .. function::
    template <typename F> \
    decltype(auto) visit(F&& fn) const

    Apply the value-visitor `fn` to the underlying value.

    :param fn: An invocable object. Must be invocable with each data type
      that an element can hold. Each overload of the invocation must return
      the same type.
    :return: Returns the value obtained by invoking the visior function with
      the appropriate value.


.. struct:: [[zero_initializable]] bson_value

  An owning dynamically-typed BSON value.

  :zero-initialized: |attr.zero-init| Represents a lack of any value. Calling
    `bson_value_delete` on such an object is a no-op. Creating a `bson_value_ref`
    from such a value will create a null `bson_value_ref`. Using a zero-initialized
    `bson_value` for most other operations is undefined behavior.

  .. note:: This is a union-like type. Only one of the value fields is active
    at a time. Check `type` before accessing other fields.

  .. note::

    For simple built-in types and trivial aggregate types, does not allocate any
    resources.

  .. member:: bson_type type

    The active type of the stored value. The special type tag `bson_type_eod`
    represents a null value.

  .. union:: __anon

    .. note:: The following are accessed directly on the `bson_value` instance

    .. member::
      bson_eod eod
      double double_
      mlib_str utf8
      bson_view document
      bson_byte_vec binary::bytes
      uint8_t binary::subtype
      bson_oid oid
      bool bool_
      bson_datetime datetime
      mlib_str regex::rx
      mlib_str regex::options
      mlib_str dbpointer::collection
      bson_oid dbpointer::object_id
      int32_t int32
      bson_timestamp timestamp
      int64_t int64
      bson_decimal128 decimal128

      The various values that can be stored. Each field corresponds to a value
      of the `type` member.


.. type:: __bson_value_convertible

  A `__bson_value_convertible` parameter is any type which can be converted to a
  `bson_value_ref`. The following types are supported:

  .. list-table::

    - - Given
      - Result
    - - `__bson_viewable`
      - `bson_type_document`
    - - `bson_array_view`
      - `bson_type_array`
    - - `__string_convertible`
      - `bson_type_utf8`
    - - `bson_binary_view`
      - `bson_type_binary`
    - - `bson_oid`
      - `bson_type_oid`
    - - `bson_datetime`
      - `bson_type_datetime`
    - - `bson_regex_view`
      - `bson_type_regex`
    - - `bson_dbpointer_view`
      - `bson_type_dbpointer`
    - - `bson_code_view`
      - `bson_type_code`
    - - `bson_symbol_view`
      - `bson_type_symbol`
    - - `int32_t`
      - `bson_type_int32`
    - - `bson_timestamp`
      - `bson_type_timestamp`
    - - `int64_t`
      - `bson_type_int64`
    - - `bson_decimal128`
      - `bson_type_decimal128`
    - - `bson_value_ref`
      - The reference is copied.
    - - `bson_value`
      - A reference to the value is created.


Functions & Macros
##################

.. function::
  void bson_value_delete(bson_value [[transfer, nullable]] val)

  Delete any resources associated with the given BSON value.


.. function::
  bson_value_ref bson_value_ref_from(__bson_value_convertible V)

  Create a dynamically typed `bson_value_ref` that corresponds to a view of the
  given value.


.. function::
  bson_value bson_value_copy(__bson_value_convertible V)
  bson_value bson_value_copy(__bson_value_convertible V, mlib_allocator alloc)

  Create a copy of `V` stored in a dynamically typed `bson_value`. The returned
  value must eventually be destroyed.


.. _iter.coerce:
.. function::
  bool bson_value_as_bool(bson_value_ref v)
  int32_t bson_value_as_int32(bson_value_ref v, bool* okay)
  int64_t bson_value_as_int64(bson_value_ref v, bool* okay)
  double bson_value_as_double(bson_value_ref v, bool* okay)

  Coerce the given BSON value to an numeric type.

  :param it: A valid value to be decoded.
  :param okay: An optional output parameter. If the returned value corresponds
    to the decoded element value, this is set to |true|. Otherwise, this
    will be set to |false|. `bson_iterator_as_bool` always coerces, so it
    omits this parameter.

  .. rubric:: Notes on behavior:

  - For coercing a boolean element to a number, a |true| element becomes
    :cpp:`1`, and |false| becomes :cpp:`0`.
  - Coercing between double, int32, and int64 will do the coercing according to
    the language's conversion rules. (e.g. an **int64** converted to a
    **double** will first extract an `int64_t` and then convert it to a
    :cpp:`double`)
  - For coercing to a ``bool`` with `bson_value_as_bool`, the following
    applies:

    - If the element is a numeric type, a |true| value will be returned if
      if is not equal to :cpp:`0`. Otherwise, it returns |false|.
    - For documents, arrays, and strings, the returned value is a bool
      corresponding to whether the corresponding value is non-empty. If decoding
      a subdocument fails, returns |false|.
    - For BSON types **EOD**, **undefined**, **null**, **min-key**, and
      **max-key**, returns |false|.
    - For all other BSON types, returns |true| unconditionally.

