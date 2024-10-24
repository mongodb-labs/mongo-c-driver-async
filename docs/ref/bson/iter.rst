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

  :zero-initialized: A zero-initialized `bson_iterator` has no defined semantics
    and should not be used for any operations.

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

    :C API: `bson_iterator_data`

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
      bson_value_ref value() const

      :C API: `bson_key`, `bson_iterator_type`, and `bson_iterator_value`


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
  bson_iterator bson_begin(__bson_viewable B)
  bson_iterator bson_end(__bson_viewable B)

  Obtain a `bson_iterator` referring to the beginning or end of the given BSON
  document, respectively.

  :C++ API: Use the ``begin()`` and ``end()`` member function of the object `B`
  :param B: A BSON document to be viewed. Passed through `bson_as_view`.
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
  :param Key: A key to search for. Passed through `mlib_as_str_view`.
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
  :param Viewable: A BSON document object, passed through `bson_as_view`
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
  mlib_str_view bson_key(bson_iterator it)

  Obtain a string that views the key of the element referred-to by `it`

  :precondition: :expr:`not bson_stop(it)`.


.. function::
  bool bson_key_eq(bson_iterator it, auto K)

  Test whether the `it` element key is equal to the given string.

  :param it: The element to inspect.
  :param K: A string to compare against. Passed through `mlib_as_str_view`

  .. note:: |macro-impl|

.. function::
  bson_type bson_iterator_type(bson_iterator it)

  Get the type of the element referred-to by `it`

  :precondition: :expr:`not bson_iterator_get_error(it)`


.. function:: bson_value_ref bson_iterator_value(bson_iterator it)

  Obtain a `bson_value_ref` that views the element value referred-to by `it`.

  :precondition: :expr:`not bson_iterator_get_error(it)`


.. function::
  const bson_byte* bson_iterator_data(bson_iterator it)

  Obtain a pointer to the element data referred-to by `it`.

  :precondition: :expr:`not bson_iterator_get_error(it)`


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
