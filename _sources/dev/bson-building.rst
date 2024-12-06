#####################################
Declaratively Building BSON Documents
#####################################

|amongoc|'s BSON library contains a facility for building heirarchies of BSON
objects declaratively in a single-shot. Usage of this facility is recommended
for a few reasons:

- It will always "do the right thing" -- Building large heirarchies of data
  by-hand is tedious and error-prone.
- The declarative structure mimics the generated data very closely, and is easy
  to understand and modify.
- The declarative building system is extensible.
- The built document will allocate exactly the right amount, exactly once: It
  first assesses the required storage space before performing the build.

.. header-file:: bson/make.hpp

  Contains types used for declarative building BSON document data.

.. note:: Everything declared in this file lives in the ``bson::make`` namespace

.. namespace:: bson::make


High-Level Constructs
#####################

.. _bson.make.value-rules:

Value Rules
***********

The rules here can be used to construct element values. The special `doc` rule
is also used to construct the root document (with `doc::build`). These implement
`value_rule` (i.e. they can be used as the value of an element's key-value
pair).


.. struct:: template <element_rule... Ts> doc

  A `value_rule` that constructs a document consisting of key-value pairs. This
  is also used to construct the root document (See: `build`).

  .. seealso::

    For a list of element rules that can be used to populate a document, see:
    :ref:`bson.make.element-rules`.

  .. note:: If you pull the ``bson::parse`` namespace, they both contain a `doc`
    declaration, and you will receive a compiler error about ambiguous name lookup.
    Pull `bson::make::doc` with a :cpp:`using`, or use the fully-qualifeid name.

  .. function:: bson::document build(mlib::allocator<>)

    Construct a new `bson::document` containing the elements of this rule.


.. struct:: template <value_rule... Vs> array

  A build rule that constructs an array value within an element. Implements
  `value_rule`.

  .. note:: This constructs a fixed-length array of elements. To construct a
    variable-length arrray, use `range`


.. struct::
  template <std__ranges__forward_range R> \
    requires value_rule<std::ranges::range_reference_t<R>> \
  range

  A `value_rule` that creates a BSON array of values that are pulled from the
  given range. The range must be a range of objects that satisfy `value_rule`.

  .. note:: The given range `R` will be iterated twice: Once to calculate the expected
      byte-size of the array, and a second time to actually perform the insertion.


.. struct::
  template <typename V> \
  optional_value

  A special `value_rule` that is used to generate conditional elements. The
  value must be |ctx-conv-bool| [#ctx-conv-bool]_. If the contained evaluates to
  :cpp:`false`, then no value will be appended.

  The wrapped object must be a `value_rule` itself, or must yield a `value_rule`
  when a unary dereference :cpp:`*` is applied (e.g. a ``std::optional``
  containing a `value_rule`).


.. _bson.make.element-rules:

Element Rules
*************

These build rules are used to specify the contents of a `doc` value rule. An
element rule can insert any number of elements into a document, but usually it
will either insert only one (i.e. `pair`) or zero (i.e. `optional_pair`).

.. struct:: template <value_rule V> pair

  Unconditionally adds a single key-value pair to a document. The key is given
  as a `std__string_view` via the first argument. The value is any `value_rule`
  given as the second argument.


.. struct:: template <value_rule V> optional_pair

  A special `element_rule` that inserts a key-value pair if-and-only-if the
  value of the candidate element evaluates to a truthy value (the value rule
  `V` must be |ctx-conv-bool| [#ctx-conv-bool]_).


Low-Level Concepts
##################

.. concept:: template <typename T> element_rule

  A concept for defining build rules that insert key-value pairs into a
  document.

  .. seealso:: :ref:`bson.make.element-rules`.

  .. rubric:: Given:
  .. var::
    T rule
    bson::mutator& out

  .. rubric:: Requires:

  - :expr:`rule.append_to(out)` - Should append any number of elements to `out`
  - :expr:`rule.byte_size()` - Should return the number of bytes that will be appended


.. concept:: template <typename T> value_rule

  A build rule or value that can be used as the value in a key-value pair for a
  document or array element.

  .. seealso:: :ref:`bson.make.value-rules`.

  **Any type** that can be inserted using `bson_insert` can be used as an
  `value_rule`.

  **Other types** may also satisfy `value_rule` by meeting the below
  requirements.

  .. rubric:: Given
  .. var::
    bson::mutator& out
    std__string_view key
    T value;

  .. rubric:: Requires

  - :expr:`value.byte_size()` must return the number of bytes that the value will
    consume. This only includes bytes for the value portion of a potential element,
    not including the element key or type tag.
  - :expr:`value.append_to(out, key)` - May append a value to the document `out`
    using the key string `key`. It is also possible that no element will be
    inserted at all (e.g. the `optional_value` may not actually append
    anything).

  .. note:: Don't call the above methods directly in generic contexts. Use
    `value_byte_size` and `append_value` instead.


.. function:: std::size_t value_byte_size(value_rule auto v)

  Obtain the size, in bytes, of the value `v` when appended to a document.

.. function::
  [[1]] void append_value(mutator& doc, std__string_view key, value_rule auto v)
  [[2]] void append_value(mutator& doc, std__size_t key, value_rule auto v)

  Append a value `v` to the document `doc` under the key `key`. :cpp:`[[2]]`
  will automatically construct an integer key string without allocating memory.


Examples
########

Create an empty document
************************

::

  using namespace bson::make;
  bson::document d = doc().build(alloc);

Create a document with a single key-value pair
**********************************************

::

  bson::document d = doc(pair("foo", "bar")).build(alloc);

.. code-block:: js

  {
    foo: "bar"
  }

Create a document with an empty array
*************************************

::

  bson::document d =
    doc(pair("foo", 123),
        pair("bar", array()))
    .build(alloc);

.. code-block:: js

  {
    foo: 123,
    bar: []
  }

Create a document with an array of strings
******************************************

::

  vector<string> strs = { "foo", "bar", "baz" };
  auto d =
    doc(pair("someStrings", range(strs)))
    .build(alloc);

.. code-block:: js

  {
    someStrings: [
      "foo",
      "bar",
      "baz"
    ]
  }

Create a ``find`` Command Document
**********************************

This is similar to the document build expression use to implement the ``find``
operation in |amongoc|, where ``params`` is a set of find parameters.

::

    bson::document command = doc(
        pair("find", collection_name),
        pair("$db", database_name),
        pair("filter", filter),
        optional_pair("sort", params->sort),
        optional_pair("projection", params->projection),
        optional_pair("hint", params->hint_doc),
        optional_pair("hint", params->hint_str),
        optional_pair("skip", params->skip),
        optional_pair("limit", params->limit),
        optional_pair("batchSize", params->batch_size),
        // If the limit is set to a negative value, generate a single batch
        pair("singleBatch", params->limit < 0),
        optional_pair("comment", params->comment),
        optional_pair("maxTimeMS", ::mlib_count_milliseconds(params->max_time)),
        optional_pair("max", params->max),
        optional_pair("min", params->min),
        pair("returnKey", params->return_key),
        pair("oplogReplay", params->oplog_replay),
        pair("showRecordId", params->show_record_id),
        pair("tailable", params->cursor_type != ::amongoc_find_cursor_not_tailable),
        pair("noCursorTimeout", params->no_cursor_timeout),
        pair("awaitData", bool(params->cursor_type & ::amongoc_find_cursor_tailable_await)),
        pair("allowPartialResults", params->allow_partial_results),
        optional_pair("collation", params->collation),
        pair("allowDiskUse", params->allow_disk_use),
        optional_pair("let", params->let))
      .build(alloc);

Example output::

  {
    find: "my-collection",
    $db: "mainDb",
    filter: { _id: { $gt: 1 } },
    batchSize: 2,
    singleBatch: false,
    returnKey: false,
    oplogReplay: false,
    showRecordId: false,
    tailable: false,
    noCursorTimeout: false,
    awaitData: false,
    allowPartialResults: false,
    allowDiskUse: false,
  }

.. |ctx-conv-bool| replace:: contextually convertible to :cpp:`bool`

.. [#ctx-conv-bool]

  A type is |ctx-conv-bool| if it will convert to a :cpp:`true` or :cpp:`false`
  value in a context that expects a :cpp:`bool` expression. See: `Implicit conversions (Contextual conversions)`__

  __ https://en.cppreference.com/w/cpp/language/implicit_conversion


