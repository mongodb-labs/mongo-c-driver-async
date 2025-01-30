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

  .. function::
    iterator begin() const
    iterator end() const
    iterator find(std__string_view) const
    bson_byte* data() const
    std__uint32_t byte_size() const

    .. seealso::
      - `bson::document::begin`
      - `bson::document::end`
      - `bson::document::find`
      - `bson::document::data`
      - `bson::document::byte_size`

  .. function::
    bson_mut& get()
    const bson_mut& get() const

  .. function::
    iterator insert(iterator pos, auto pair)
    iterator insert(iterator pos, bson_iterator::reference elem)
    iterator emplace(iterator pos, std__string_view key, __bson_value_convertible auto val)

    :C API: `bson_insert`
    :invalidation: |all-invalidated|

    - :expr:`insert(pos, pair)` method is equivalent to
      :expr:`emplace(pos, std::get<0>(pair), std::get<1>(pair))`.
    - :expr:`insert(pos, elem)` is equivalent to :expr:`emplace(pos, elem.key(), elem.value())`

  .. function::
    iterator push_back(auto pair)
    iterator push_back(bson_iterator::reference elem)
    iterator emplace_back(std__string_view key, __bson_value_convertible auto val)

    :C API: `bson_insert`
    :invalidation: |all-invalidated|

    These are equivalent to `insert` or `emplace` with the end iterator as the
    insertion position.

  .. function::
    inserted_subdocument insert_subdoc(iterator pos, std__string_view key)
    inserted_subdocument insert_array(iterator pos, std__string_view key)
    mutator push_subdoc(std__string_view key)
    mutator push_array(std__string_view key)

    Insert a new empty child document or array, and obtain a mutator for that
    child.

    .. important:: |mut-stable-admon|

  .. struct:: inserted_subdocument

    .. member::
      iterator position
      mutator mut

  .. function::
    mutator child(iterator pos)
    iterator parent_iterator() const

    :C API:
      - `bson_mut_child`
      - `bson_mut_parent_iterator`

    .. important:: Regarding `child`: |mut-stable-admon|


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

  Obtain a new `bson_mut` |M| mutator for the given document object.

  :invalidation: No iterators are |invalidated| by this call, but subsequent
    operations on |M| may potentially invalidate them.

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
  :invalidation: |all-invalidated|

  .. note:: |macro-impl|

  .. rubric:: Value Types

  The type of the newly inserted value is determined according to the
  `__bson_value_convertible` type rules.


.. function::
  bson_mut bson_mut_child(bson_mut* parent, bson_iterator pos)

  Obtain a mutator |M| that manipulates a child document element at position
  `pos` within `parent`.

  :param parent: An existing mutator that refers to the document that owns `pos`.
  :param pos: An iterator referring to a document or array element within `parent`.
  :invalidation: No iterators are |invalidated| by this function, but subsequent
    operations may invalidate them. Use `bson_mut_parent_iterator` to recovery
    the iterator `pos` from |M|.

  .. important:: |mut-stable-admon|


.. function::
  bson_iterator bson_mut_parent_iterator(bson_mut m)

  Obtain a `bson_iterator` that refers to the position of `m` within a parent
  document. This can only be called on a `bson_mut` that was created as a child
  of another `bson_mut`.

  :param m: A mutator object that was returned by `bson_mut_child`. Calling this
    with a mutator returned `bson_mutate` is *undefined behavior*.
  :invalidation: No iterators are |invalidated|.

  This is useful to recover an iterator referring to a child document element
  after mutating that child document, since mutating a child may invalidate
  iterators in the parent.


Removing Elements
*****************

.. function::
  [[1]] bson_iterator bson_erase(bson_mut* m, bson_iterator pos)
  [[2]] bson_iterator bson_erase(bson_mut* m, bson_iterator first, bson_iterator last)

  Erase one or multiple elements within a document `m`.

  :param m: A mutator for the document to be modified.
  :param pos: A valid iterator referring to the single element to be erased.
  :param first: The first element to be erased.
  :param last: The first element to be retained, or the end iterator.
  :return:
    1. The updated iterator referring to the position after `pos`
    2. The updated iterator referring to the `last` position
  :invalidation:
    1. |all-invalidated|
    2. `bson_erase_range` will |invalidate| all reachable iterators |iff|
       `first` is not equal to `last`.

  If `first` and `last` are equivalent, then no element will be removed.

  .. note:: |macro-impl|


Modifying Elements
******************

Existing document elements can be modified in-place to a limited extent.

.. function::
  bson_iterator bson_set_key(bson_mut* mut, bson_iterator pos, __string_convertible new_key)

  Replace the element key of the element pointed-to by `pos`.

  :param mut: Mutator for the document owning `pos`
  :param pos: A valid iterator pointing to a live element. Must not be an error
    or end iterator.
  :param new_key: The key string to be replaced.
  :return: The adjusted `pos` iterator following the modification.
  :invalidation: |all-invalidated| |iff| `new_key` is not the same
    length as :expr:`bson_key(pos)`.


.. function::
  bson_iterator bson_relabel_array_elements_at(bson_mut* doc, bson_iterator pos, uint32_t idx)
  void bson_relabel_array_elements(bson_mut* doc)

  Relabel elements within a BSON document `doc` as monotonically increasing
  decimal integers. `bson_relabel_array_elements` is equivalent to
  :expr:`bson_relabel_array_elements_at(doc, bson_begin(*doc), 0)`.

  :param doc: The document to be modifed.
  :param pos: Iterator referring to the first element to be modified. If equal
    to :expr:`bson_end(*doc)`, then no elements are modified.
  :param idx: The integer key to set for `pos`. All subsequent elements will be
    relabeled by incrementing this index for each element.
  :return: Returns the iterator referring to the `pos` element after the
    relabelling is complete.
  :invalidation: |all-invalidated| |iff| the length of any element's new key is
    not equal to the length of its existing key. (When in doubt, assume all
    iterators are invalidated.)


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
  :return: Returns the adjusted iterator pointing to the `pos` element.
  :invalidation: |all-invalidated| |iff| any elements are deleted or inserted
    (i.e. :expr:`pos != delete_end or from_begin != from_end`)

  .. important::

    If `from_begin` and `from_end` are not equal, then `from_begin` and
    `from_end` MUST NOT be elements within `m` or any elements within its
    document heirarchy.

  .. note::

    `delete_end` must be reachable from `pos`, and `from_end` must be reachable
    from `from_begin`.


.. function::
  bson_iterator bson_insert_disjoint_range(bson_mut* doc, bson_iterator pos, bson_iterator from_begin, bson_iterator from_end)

  Copy elements in the range :cpp:`[from_begin, from_end)` into the document
  `doc` at `pos`.

  Equivalent to :expr:`bson_splice_disjoint_ranges(doc, pos, pos, from_begin, from_end)`

  :invalidation: |all-invalidated| |iff| :expr:`from_begin != from_end`


Utilities
*********

.. struct:: bson_u32_string

  .. member:: char buf[11]

    A small string enough to encode a non-negative 32-bit integer with a null
    terminator.

.. function::
  bson_u32_string bson_u32_string_create(uint32_t i)

  Create a small :term:`C string` representing the base-10 encoding of the given
  32-bit integer `i`. The string is not dynamically allocated, so no
  deallocation is necessary. The character array in the returned small string is
  null-terminated.


Behavioral Notes
################

.. _mut.iter.invalidate:

Iterator Invalidation
*********************

A BSON iterator |I| is *reachable* for a `bson_doc` |D| or `bson_mut` derived
from |D| if there is any way to obtain that iterator by traversing the document
heirarchy. |I| may be an iterator at the top level, or may be an iterator within
any sub-document of |D|.

A BSON iterator |I| that *reachable* in a `bson_doc` |D| may be *invalidated* by
certain operations on |D| or any sub-document thereof. **This is true even if**
the operation does not cause a reallocation! For this reason, the insertion,
erasing, and splicing APIs all return iterators that are adjusted to account for
the invalidating operations. Iterator invalidation behaviors are documented
under the **Invalidation** field on the relevant function.

After modifying a subdocument |D'| using `bson_mut_child`, an iterator referring
to |D'| can be recovered by using `bson_mut_parent_iterator` on the mutator that
was created with `bson_mut_child`.

.. |invalidate| replace:: :ref:`invalidate <mut.iter.invalidate>`
.. |invalidated| replace:: :ref:`invalidated <mut.iter.invalidate>`
.. |all-invalidated| replace:: All reachable iterators are |invalidated|
