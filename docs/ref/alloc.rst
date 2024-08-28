#####################
Header: |this-header|
#####################

.. |this-header| replace:: :header-file:`amongoc/alloc.h`
.. header-file:: amongoc/alloc.h

  Contains types, functions, and constants related to dynamic memory management.

.. struct:: amongoc_allocator

  Provides support for customizing dynamic memory allocation.

  :header: |this-header|

  Many |amongoc| APIs accept an `amongoc_allocator` parameter, or otherwise
  consult other objects that carry an `amongoc_allocator` (e.g. `amongoc_loop`
  has an associated allocator, and therefore any object associated with an event
  loop will also use that same allocator).

  .. member:: void* userdata

    Arbitrary pointer to context for the allocator.

  .. function:: void* reallocate(void* userdata, void* prev_ptr, std::size_t requested_size, std::size_t previous_size, std::size_t* [[storage]] out_new_size)

    **(Function pointer member)**

    .. note:: Don't call this directly. Use `amongoc_allocate` and `amongoc_deallocate`

    Implements custom allocation for the allocator. The user must provide a
    non-null pointer to a function for the allocator.

    :param userdata: The `amongoc_allocator::userdata` pointer.
    :param prev_ptr: Pointer that was previously returned by `reallocate`
    :param requested_size: The requested amount of memory for the new region, **or**
      zero to request deallocation of `prev_ptr`.
    :param previous_size: If `prev_ptr` is not :cpp:`nullptr`, this is the
      previous `requested_size` used when `prev_ptr` was allocated.
    :param out_new_size: An output parameter: The allocator will write the actually
      allocated size to this pointer. The argument may be :cpp:`nullptr`.

    .. rubric:: Allocation Behavior

    In brief:

    1. If `requested_size` is zero: Behave as-if ``free(prev_ptr)``, set
       ``*out_new_size`` to zero.
    2. Otherwise, behave as-if ``realloc(prev_ptr, requested_size)``, set
       ``*out_new_size`` to `requested_size`.

    In greater detail, a user-provided allocation function must do the
    following:

    .. |Rp| replace:: :math:`R_p`
    .. |Sp| replace:: :math:`S_p`

    3. If `prev_ptr` is null:

       1. If `requested_size` is zero, return a null pointer.
       2. Attempt to allocate a region |R| of size |S| bytes, where |S| is at
          least `requested_size`.
       3. If allocating |R| fails, return a null pointer.
       4. If `out_new_size` is not null, write |S| to ``*out_new_size``.
       5. Return a pointer to the beginning of |R|.

    4. Otherwise (`prev_ptr` is non-null), let |Rp| be the existing memory
       region of size |Sp| pointed-to by `prev_ptr`.

    5. If `requested_size` is zero:

       1. Release the region |Rp|.
       2. If `out_new_size` is not null, write zero to ``*out_new_size``.
       3. Return a null pointer.

    6. Otherwise (`requested_size` is non-zero), if `requested_size` is greater
       than |Sp|:

       1. Attempt to grow the region |Rp| to a new size |S| where |S| is at
          least `requested_size` bytes. If successful:

          1. If `out_new_size` is not null, write |S| to ``*out_new_size``.
          2. Return a pointer to |Rp|.

       2. Otherwise (growing the region failed), attempt to allocate a new region |R|
          of size |S|, where |S| is at least `requested_size` bytes. If succesful:

          1. Copy the first |Sp| bytes from |Rp| into |R|.
          2. Release the region |Rp|.
          3. If `out_new_size` is not null, write |S| to ``*out_new_size``.
          4. Return a pointer to the beginning of |R|.

       3. Otherwise (allocating a new region failed), return a null pointer. (Do
          not modify the region |Rp| nor write anything to `out_new_size`)

    7. Otherwise (`requested_size` is non-zero and at most |Sp|):

       1. Optionally, to simply reuse the region |Rp|:

          1. If `out_new_size` is not null, write |Sp| to ``*out_new_size``
          2. Return `prev_ptr` unmodified.

       2. Otherwise, attempt to shrink the region |Rp| to a new size |S| that is
          at least `requested_size` bytes. If successful:

          1. If `out_new_size` is not null, write |S| to ``*out_new_size``.
          2. Return a pointer to |Rp|.

       3. Otherwise (shrinking the region failed), attempt to allocate a new
          region |R| of size |S|, where |S| is at least `requested_size`. If
          succesful:

          1. Copy the first `requested_size` bytes from |Rp| into |R|.
          2. If `out_new_size` is not null, write |S| into ``*out_new_size``.
          3. Release the region |Rp|.
          4. Return a pointer to the beginning of |R|.

       4. Otherwise (allocating a new region failed), return a null pointer (Do
          not modify the region |Rp| nor write anything to `out_new_size`).


.. function::
  void* amongoc_allocate(amongoc_allocator alloc, std::size_t sz)
  void amongoc_deallocate(amongoc_allocator alloc, void* p, std::size_t sz)

  Attempt to allocate or deallocate memory using the allocator `alloc`.

  :param alloc: The allocator to be used.
  :param p: (For deallocation) A pointer that was previously returned by `amongoc_allocate`
    using the same `alloc` parameter.
  :param sz: For allocation, the requested size. For deallocation, this must be
    the original `sz` value that was used with `amongoc_allocate`.
  :header: |this-header|


.. cpp:var:: const amongoc_allocator amongoc_default_allocator

  A reasonable default `amongoc_allocator`.

  :header: |this-header|

  This allocator is implemented in terms of the standard library
  :cpp:`realloc()` and :cpp:`free()` functions.


.. cpp:var:: const amongoc_allocator amongoc_terminating_allocator

  A special `amongoc_allocator` that terminates the program if there is any
  attempt to allocate memory through it.

  :header: |this-header|

  This allocator is intended to be used in places where the programmer wishes to
  assert that dynamic allocation will not occur. If an attempt is made to
  allocate memory using this alloator, then a diagnostic will be printed to
  standard error and :cpp:`abort()` will be called.


.. namespace:: amongoc

.. class:: template <typename T = void> cxx_allocator

  Provides a C++ allocator interface for an `amongoc_allocator`

  :header: |this-header|

  This allocator type is *not* default-constructible: It must be constructed
  explicitly from an `amongoc_allocator`.

  .. type::
      value_type = T
      pointer = value_type*

      Types associated with this allocator.

      .. note:: If `T` is ``void``, then the allocator is a proto-allocator and
          must be converted to a typed allocator before it may be used.

  .. function::
    cxx_allocator(amongoc_allocator a)

    Construct from an `amongoc_allocator` `a`.

  .. function::
    template <typename U> cxx_allocator(cxx_allocator<U>)

    Convert-construct from another allocator instance, rebinding the allocated
    type.

  .. function:: bool operator==(cxx_allocator) const

    Compare two allocators. Two `cxx_allocator`\ s are equal if the
    `amongoc_allocator::userdata` and `amongoc_allocator::reallocate` pointers
    are equal.

  .. function:: amongoc_allocator c_allocator() const

    Obtain the `amongoc_allocator` that is used by this `cxx_allocator`

  .. function::
    pointer allocate(std::size_t n) const
    void deallocate(pointer p, std::size_t n) const

    The allocation/deallocation functions for the C++ allocator interface.

    :param n: The number of objects to be allocated/deallocated
    :param p: Pointer to a previous region obtained from an equivalent `cxx_allocator`

    Calls `amongoc_allocate`/`amongoc_deallocate` to perform the allocation.


.. var:: const cxx_allocator<> terminating_allocator{::amongoc_terminating_allocator}

  A C++ version of the `amongoc_terminating_allocator`

  :header: |this-header|