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
  :zero-initialized: A zero-initialized `bson_mut` has no defined semantics and
    should not be used.
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

  .. important:: |mut-stable-admon|


Inserting Data
**************

.. function::
  [[1]] bson_iterator bson_insert(bson_mut* m, __string_convertible key, __bson_value_convertible value)
  [[2]] bson_iterator bson_insert(bson_mut* m, bson_iterator pos, __string_convertible key, __bson_value_convertible value)

  Insert a a value into a BSON document referred-to by `m`.

  :param m: A BSON mutator for the document to be updated.
  :param pos: A position at which to perform the insertion. For version ``[[1]]``,
    the default position is :expr:`bson_end(*m)`, which will append the value to
    the end of the document.
  :param key: The new element key.
  :param value: A value to be inserted.
  :return: Upon success, returns an iterator that refers to the inserted element.
    If there is an allocation failure, returns :expr:`bson_end(*m)`.

  .. note:: |macro-impl|

  .. rubric:: Value Types

  The following value types are supported automatically by `bson_insert`. The
  type of the newly inserted value is determined according to the
  `__bson_value_convertible` type rules.


.. function::
  bson_mut bson_mut_child(bson_mut* parent, bson_iterator pos)

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

A BSON iterator |I| that belongs to a `bson_doc` |D| or and sub-document of |D|
is *invalidated* if *any* elements are added or removed within the document
heirarchy of |D|. **This is true even if** the operation does not cause a
reallocation! For this reason, the insertion, erasing, and splicing APIs all
return iterators that are adjusted to account for the invalidating operations.

After modifying a subdocument |D'| using `bson_mut_child`, an iterator referring
to |D'| can be recovered by using `bson_mut_parent_iterator` on the mutator that
was created with `bson_mut_child`.
