##############
BSON Documents
##############

.. header-file:: bson/doc.h

  Contains types and utilities for creating and copying BSON documents.

.. |this-header| replace:: :header-file:`bson/doc.h`


Types
#####

.. struct:: bson_doc

  A struct that owns a top-level BSON document. This type is trivially
  relocatable.

  :C++ API: `bson::document` should be preferred in C++ code, as it will
    automatically copy and destroy document objects.
  :header: |this-header|

  A `bson_doc` has three significant properties:

  - A `bson_doc` owns a pointer to an array of bytes that represent the document
    data itself. This can be accessed using `bson_data`, `bson_mut_data`, and
    `bson_size`.
  - A `bson_doc` has an associated `mlib_allocator` which is given when it was
    constructed. This can be obtained with `bson_doc_get_allocator`.
  - A `bson_doc` has a *capacity*, which is the number of bytes allocated for
    document data. When attempting to mutate a document beyond its capacity,
    it will reallocate the underlying memory buffer. This can be queried
    with `bson_doc_capacity` and manipulated using `bson_doc_reserve`.


.. class:: bson::document

  This class wraps a `bson_doc` in a C++ interface that will automatically copy
  and delete the underlying data.

  :C API: `bson_doc`
  :header: |this-header|


Functions & Macros
##################

Inspecting & Iterating
**********************

Note that several functions/macros useful with `bson_doc` are documented on
the :doc:`view` and :doc:`iter` page, including:

- `bson_begin`
- `bson_end`
- `bson_size`
- `bson_data`
- `bson_as_view`

.. function::
  uint32_t bson_doc_capacity(bson_doc d)

  Obtain the capacity of the given BSON document.

  :header: |this-header|


.. function::
  mlib_allocator bson_doc_get_allocator(bson_doc d)

  Obtain the allocator associated with `d`

  :header: |this-header|


Create & Deletion
*****************

.. function::
  [[1]] bson_doc bson_new(uint32_t reserve, mlib_allocator alloc)
  [[2]] bson_doc bson_new(auto doc, mlib_allocator alloc)
  [[3]] bson_doc bson_new()
  [[4]] bson_doc bson_new(uint32_t reserve)
  [[5]] bson_doc bson_new(mlib_allocator alloc)
  [[6]] bson_doc bson_new(bson_doc doc)
  [[7]] bson_doc bson_new(auto doc)

  Create a new `bson_doc`.

  :param reserve: The number of bytes to reserve for the new document. The
    default and minimum is 5 bytes.
  :param doc: A document to be copied.
  :param alloc: An allocator to be used with the docum
  :header: |this-header|

  .. rubric:: Overloads

  1. Creates a new document with `reserve` bytes of capacity using the given
     allocator `alloc`.
  2. Copies the data from `doc` into a new document that is created as-if by
     :expr:`bson_new(bson_size(doc), alloc)`
  3. Equivalent to :expr:`bson_new(5, mlib_default_allocator)`
  4. Equivalent to :expr:`bson_new(reserve, mlib_default_allocator)`
  5. Equivalent to :expr:`bson_new(5, alloc)`
  6. Equivalent to :expr:`bson_new(bson_as_view(doc), bson_doc_get_allocator(doc))`.
  7. Equivalent to :expr:`bson_new(bson_as_view(doc), mlib_default_allocator)`

  If the reserved size is five bytes (the default), then this function will not
  allocate any memory. It will only allocate memory if there is an attempt to
  insert additional data.

  When finished, the returned object should be given to `bson_delete`

  .. note:: The actual allocated size will be slightly larger than the requested
    size as `bson_doc` requires additional bookkeeping data.

  .. note:: |macro-impl|


.. function::
  void bson_delete(bson_doc [[transfer]] doc)

  Delete a previously created `bson_doc` object.

  :param doc: |attr.transfer| The document to be destroyed.


.. function::
  int32_t bson_doc_reserve(bson_doc* doc, uint32_t size)

  Adjust the capacity of the document `doc`.

  :param doc: Pointer to a document to be updated.
  :param size: The new capacity. If this is less than the current capacity, this
    function does nothing.
  :return: Upon success, returns the new capacity of the document. If allocation
     fails, returns a negative value.

