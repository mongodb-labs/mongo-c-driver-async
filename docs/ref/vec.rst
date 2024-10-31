############
Vector Types
############

|amongoc| defines various dynamically-sized contiguous array types. These are
stamped out from a template and all follow an identical API shape. These vector
types are conventially named as ``xyz_vec``, where ``xyz`` is the name of the
element type.

This page will document the vector type interface using a placeholder type
`item_vec::T`.

.. warning::

  The vector API template is contained in a publicly-visible header, but direct
  use of that header is an implementation detail.


.. struct:: [[zero_initializable]] item_vec

  A dynamically-sized contiguous array of objects of type `T`. **Note** that
  the name `item_vec` is a stand-in for an actual concrete vector type.

  .. type:: T

    The name "`T`" herein acts as a stand-in for the type of the actual vector
    elements, which vary.

  :zero-initialized: |attr.zero-init| A zero-initialized vector is considered
    *null*, and holds no objects. Deleting a null vector is a no-op. A null
    vector cannot be resized: It must first be initialized using `item_vec_new`.

  A vector type will automatically call a pre-defined destructor function on
  the vector elements when it is resized or deleted.


  .. member:: T* data

    A pointer to the first element of the contiguous array. The length of the
    pointed-to array is designated by `size`.


  .. member:: size_t size

    The length of the array pointed-to by `data`


  .. member:: mlib_allocator allocator

    The dynamic memory allocator associated with this object.


  .. function::
    T* begin()
    T* end()

    Allows use as a C++ range


.. function::
  item_vec::T* item_vec_begin(item_vec vec)
  item_vec::T* item_vec_end(item_vec vec)

  Obtain the begin/end pointers to the dynamically-sized array managed by `vec`.


.. function:: size_t item_vec_max_size(void)

  Obtain the maximum number of objects that can be stored in an `item_vec`


.. function::
  [[1]] item_vec item_vec_new(mlib_allocator alloc)
  [[2]] item_vec item_vec_new_n(size_t n, bool* alloc_okay, mlib_allocator alloc)

  Create a new vector of objects. ``[[1]]`` Creates a new empty vector.
  ``[[2]]`` is equivalent to calling ``[[1]]`` followed by an immediate
  `item_vec_resize`.

  :param n: The number of objects to be created.
  :param alloc_okay: A boolean that is set to |true| when the function succeeds.
    A |false| result indicates an allocation failure. This must not be null, as
    this is the only reliable way to detect allocation failures.
  :param alloc: The allocator for the new vector.


.. function::
  void item_vec_delete(item_vec [[transfer, nullable]] self)

  Destroy all elements are release any store associated with `self`.


.. function:: bool item_vec_resize(item_vec* self, size_t count)

  Resize the vector `self` to contain `count` items. Removed items are
  destroyed. Returns |true| on success, returns |false| if there was an
  allocation failure.


.. function:: item_vec::T* item_vec_push(item_vec* self)

  Append a new item to the end of the vector. Upon success, returns a pointer to
  the added object. If there is an allocation failure, then a null pointer is
  returned.
