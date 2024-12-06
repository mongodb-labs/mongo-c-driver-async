###########
Adding Data
###########

:doc:`The previous tutorial <lifetime>` covered how to create and destroy
document objects, but we'll eventually want to insert content. This page will
discuss how to insert data into a document object.


Getting a Mutator
#################

Modifying a `bson_doc` can't be done directly. We need to obtain a `bson_mut`
*mutator* for a document using `bson_mutate`:

.. literalinclude:: bson.example.cpp
  :start-after: [mutate]
  :end-before: end.

.. important::

  A `bson_mut` object retains a pointer to the `bson_doc` from which it was
  created. The origin `bson_doc` must remain in place while the `bson_mut`
  object is being used!

The `bson_mut` object does not need to be destroyed or finalized. Mutations
performed are applied to the original `bson_doc` immediately as they execute.


Inserting Values
################

Values can be inserted into a mutator using the `bson_insert` generic function
type:

.. literalinclude:: bson.example.cpp
  :start-after: [insert]
  :end-before: end.

The `bson_insert` function supports multiple signatures. The element type will
be inferred from the final argument, which is the value to insert. Refer to its
documentation page for full details.


Inserting in Other Positions
############################

Values can be inserted into the middle or front of a document, and the existing
elements will be rearranged appropriately. This is done by passing an iterator
to `bson_insert` as the second argument:

.. literalinclude:: bson.example.cpp
  :start-after: [insert-begin]
  :end-before: end.

When no position is given to `bson_insert`, it will implicitly insert the
element at the end.

Working with iterators will be covered in the next page.


Inserting in a Child Document
#############################

`bson_mut` supports consrtucting a heirarchy of document objects. After
inserting a new document, the returned iterator can be used to obtained a
`bson_mut` that manipulates a child document:

.. literalinclude:: bson.example.cpp
  :start-after: [subdoc-mutate]
  :end-before: end.

.. note:: The iterator that refers to the child document will be invalidated
  when the child document is modified. To recover a valid iterator, use
  `bson_mut_parent_iterator`.
