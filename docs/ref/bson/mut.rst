##############
BSON Modifying
##############

.. header-file:: bson/mut.h

  Contains types and APIs for manipulating `bson_doc` objects.

.. |this-header| replace:: :header-file:`bson/mut.h`

.. |mut-stable-admon| replace::

  The mutator retains a pointer to the object from which it was created. It is
  essential that the parent object remain in-place while a mutator is in use.

Types
#####

.. struct:: bson_mut

  A BSON document mutator.

  :C++ API: `bson::mutator`
  :header: |this-header|

  A `bson_mut` is created using `bson_mutate` or may come from another API that
  derives a new `bson_mut` from an existing `bson_mut`.

  .. important:: |mut-stable-admon|


.. class:: bson::mutator

  A BSON document mutator.

  :C API: `bson_mut`

  .. important:: |mut-stable-admon|

  .. function::
    mutator(document&)
    mutator(bson_doc&)

    Creates a new mutator for the given object.


Functions & Macros
##################

The document iteration/reading APIs detailed in :doc:`iter` and :doc:`view` also
work with `bson_mut` objects, including:

- `bson_begin`
- `bson_end`
- `bson_size`
- `bson_mut_data`

.. function::
  bson_mut bson_mutate(bson_doc*)

  Obtain a new `bson_mut` mutator for the given document object.


Inserting Data
**************

.. function::
  [[1]] bson_iterator bson_insert(bson_mut* m, auto key, auto value)
  [[2]] bson_iterator bson_insert(bson_mut* m, bson_iterator pos, auto key, auto value)

  Insert a a value into a BSON document referred-to by `m`.

  :param m: A BSON mutator for the document to be updated.
  :param pos: A position at which to perform the insertion. For version ``[[1]]``,
    the default position is :expr:`bson_end(*m)`, which will append the value to
    the end of the document.
  :param key: The new element key. Will be passed through `bson_as_utf8`.
  :param value: A value to be inserted. Must be one of the supported value types.
  :return: Upon success, returns an iterator that refers to the inserted element.
    If there is an allocation failure, returns :expr:`bson_end(*m)`.

  .. note:: |macro-impl|

  .. rubric:: Value Types

  The following value types are supported automatically by `bson_insert`:

  - `int32_t` and `int64_t`
  - ``double`` and ``float``
  - ``const char*`` and `bson_utf8_view`
  - `bson_view`, `bson_doc`, and `bson_mut` (will insert as a sub-document).
  - `bson_binary`
  - `bson_oid`
  - ``bool``
  - `bson_datetime`
  - `bson_regex`
  - `bson_dbpointer`
  - `bson_code`
  - `bson_symbol`
  - `bson_timestamp`
  - `bson_decimal128`

  To insert a unit typed value (e.g. ``null``), use the typed insertion
  functions (e.g. `bson_insert_null`).


.. function::
  bson_iterator bson_insert_double(bson_mut d, bson_iterator p, auto key, double dbl)
  bson_iterator bson_insert_utf8(bson_mut d, bson_iterator p, auto key, auto u8)
  bson_iterator bson_insert_doc(bson_mut d, bson_iterator p, auto key, auto subdoc)
  bson_iterator bson_insert_array(bson_mut d, bson_iterator p, auto key)
  bson_iterator bson_insert_binary(bson_mut d, bson_iterator p, bson_binary bin)
  bson_iterator bson_insert_undefined(bson_mut d, bson_iterator p, auto key)
  bson_iterator bson_insert_oid(bson_mut d, bson_iterator p, auto key, bson_oid oid)
  bson_iterator bson_insert_bool(bson_mut d, bson_iterator p, auto key, bool b)
  bson_iterator bson_insert_datetime(bson_mut d, bson_iterator p, auto key, bson_datetime dt)
  bson_iterator bson_insert_null(bson_mut d, bson_iterator p, auto key)
  bson_iterator bson_insert_regex(bson_mut d, bson_iterator p, auto key, bson_regex rx)
  bson_iterator bson_insert_dbpointer(bson_mut d, bson_iterator p, auto key, bson_dbpointer dbp)
  bson_iterator bson_insert_code(bson_mut d, bson_iterator p, auto key, bson_code code)
  bson_iterator bson_insert_symbol(bson_mut d, bson_iterator p, auto key, bson_symbol sym)
  bson_iterator bson_insert_int32(bson_mut d, bson_iterator p, auto key, int32_t i)
  bson_iterator bson_insert_timestamp(bson_mut d, bson_iterator p, auto key, bson_timestamp ts)
  bson_iterator bson_insert_int64(bson_mut d, bson_iterator p, auto key, int64_t i)
  bson_iterator bson_insert_decimal128(bson_mut d, bson_iterator p, auto key, bson_decimal128 dec)

  Insert a value of the corresponding type into the document `d` at the given
  position `p`.

  :param d: A `bson_mut` mutator for some BSON document.
  :param p: A `bson_iterator` that refers to some position. The new element will
    be inserted at the position `p`. If `p` points to an existing element, then
    the new element will appear *before* the element at `p`.
  :param key: The new element key. Passed through `bson_as_utf8`


.. function::
  bson_mut bson_child_mut(bson_mut* parent, bson_iterator pos)

  Obtain a mutator that manipulates a child document element at position `pos`
  within `parent`.

  :param parent: An existing mutator that refers to the document that owns `pos`.
  :param pos: An iterator referring to a document or array element within `parent`.

  .. important:: |mut-stable-admon|


.. function::
  bson_iterator bson_mut_parent_iterator(bson_mut m)

  Obtain a `bson_iterator` that refers to the position of `m` within a parent
  document. This can only be called on a `bson_mut` that was created as a child
  of another `bson_mut`.

  This is useful to recover an iterator referring to a child document element
  after mutating that child document, since mutating a child may invalidate
  iterators in the parent.


Removing Elements
*****************

.. function::
  bson_iterator bson_erase(bson_mut* m, bson_iterator pos)
  bson_iterator bson_erase_range(bson_mut* m, bson_iterator first, bson_iterator last)

  Erase one or multiple elements within a document `m`.

  :param m: A mutator for the document to be modified.
  :param pos: A valid iterator referring to the single element to be erased.
  :param first: The first element to be erased.
  :param last: The first element to be retained, or the end iterator.
  :return: Returns an iterator referring to the position after the removal.

  If `first` and `last` are equivalent, then no element will be removed.


Splicing Ranges
***************

.. function::
  bson_iterator bson_splice_disjoint_ranges(bson_mut* m, bson_iterator pos, bson_iterator delete_end, bson_iterator from_begin, bson_iterator from_end)

  Delete elements from `m` and insert elements from another document into their place.

  :param m: The document being modifed.
  :param pos: The position at which the splice operation will occur.
  :param delete_end: The first element after `pos` which will not be deleted. If
    equal to `pos`, then no elements will be erased.
  :param from_begin: The first element to copy into `pos`
  :param from_end: The end of the range from which to copy.

  .. important::

    If `from_begin` and `from_end` are not equal, then `from_begin` and
    `from_end` MUST NOT be elements within `m` or any elements within its
    document heirarchy.

  .. note::

    `delete_end` must be reachable from `pos`, and `from_end` must be reachable
    from `from_begin`.


Behavioral Notes
################

Iterator Invalidation
*********************

A BSON iterator |I| that belongs to a `bson_doc` |D| is *invalidated* if *any*
elements are added or removed within the document heirarchy of |D|.

For this reason, the insertion, erasing, and splicing APIs all return iterators
that are adjusted to account for the invalidating operations.

After modifying a subdocument |D'| using `bson_mut_child`, an iterator referring
to |D'| can be recovered by using `bson_mut_parent_iterator` on the mutator
created with `bson_mut_child`.

