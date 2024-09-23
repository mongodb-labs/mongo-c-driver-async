#####################
Header: |this-header|
#####################

.. |this-header| replace:: :header-file:`mlib/alloc.h`
.. header-file:: mlib/alloc.h

  Contains types, functions, and constants related to dynamic memory management.

C APIs
######

Types
*****

.. struct::
  mlib_allocator

  Provides support for customizing dynamic memory allocation.

  :header: |this-header|

  Many |amongoc| APIs accept an `mlib_allocator` parameter, or otherwise consult
  other objects that carry an `mlib_allocator` (e.g. `amongoc_loop` has an
  associated allocator, and therefore any object associated with an event loop
  will also use that same allocator).

  .. member:: mlib_allocator_impl const* impl

    Pointer to the allocator implementation.

.. struct:: mlib_allocator_impl

  Provides the backing implementation for an `mlib_allocator`

  .. member:: void* userdata

    Arbitrary pointer to context for the allocator.

  .. function:: void* reallocate(void* userdata, void* prev_ptr, std::size_t requested_size, std::size_t alignment, std::size_t previous_size, std::size_t* [[storage]] out_new_size)

    **(Function pointer member)**

    .. note:: Don't call this directly. Use `mlib_allocate`, `mlib_deallocate`, and `mlib_reallocate`

    Implements custom allocation for the allocator. The user must provide a
    non-null pointer to a function for the allocator.

    :param userdata: The `mlib_allocator::userdata` pointer.
    :param prev_ptr: Pointer that was previously returned by `reallocate`
    :param requested_size: The requested amount of memory for the new region, **or**
      zero to request deallocation of `prev_ptr`.
    :param alignment: The requested alignment of the new memory region. Must be
      a power of two.
    :param previous_size: If `prev_ptr` is not :cpp:`nullptr`, this is the
      previous `requested_size` used when `prev_ptr` was allocated.
    :param out_new_size: An output parameter: The allocator will write the actually
      allocated size to this pointer. The argument may be :cpp:`nullptr`.
    :return:
      - **Upon success**, must return a pointer to the newly allocated region of
        size at least `requested_size` with alignment of at least `alignment`.
      - **Upon failure**, must return :cpp:`nullptr`.

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


Functions
*********

.. function::
  void* mlib_allocate(mlib_allocator alloc, std::size_t sz)
  void mlib_deallocate(mlib_allocator alloc, void* p, std::size_t sz)
  void* mlib_reallocate(mlib_allactor alloc, void* prev_ptr, std::size_t sz, std::size_t alignment, std::size_t prev_size, std::size_t* out_new_size)

  Attempt to allocate or deallocate memory using the allocator `alloc`.

  :param alloc: The allocator to be used.
  :param p: (For deallocation) A pointer that was previously returned by `mlib_allocate`
    using the same `alloc` parameter.
  :param sz: For allocation, the requested size. For deallocation, this must be
    the original `sz` value that was used with `mlib_allocate`.
  :return:
    - For allocation functions: **upon success**: returns a pointer to the
      beginning of a newly allocated region of at least size `sz` and optional
      alignment `alignment`. **Upon failure**, returns :cpp:`nullptr`.

  :header: |this-header|

  The `mlib_reallocate` function is a wrapper around the
  `mlib_allocator_impl::reallocate` function.


Constants
*********

.. cpp:var:: const mlib_allocator amongoc_default_allocator

  A reasonable default `mlib_allocator`.

  :header: |this-header|

  This allocator is implemented in terms of the standard library
  :cpp:`realloc()` and :cpp:`free()` functions.


.. cpp:var:: const mlib_allocator amongoc_terminating_allocator

  A special `mlib_allocator` that terminates the program if there is any
  attempt to allocate memory through it.

  :header: |this-header|

  This allocator is intended to be used in places where the programmer wishes to
  assert that dynamic allocation will not occur. If an attempt is made to
  allocate memory using this alloator, then a diagnostic will be printed to
  standard error and :cpp:`abort()` will be called.


C++ APIs (Namespace ``mlib``)
#############################

.. namespace:: mlib

Types
*****

.. class:: template <typename T = void> allocator

  Provides a C++ allocator interface for an `mlib_allocator`

  :header: |this-header|

  This allocator type is *not* default-constructible: It must be constructed
  explicitly from an `mlib_allocator`.

  .. type::
      value_type = T
      pointer = value_type*

      Types associated with this allocator.

      .. note:: If `T` is ``void``, then the allocator is a proto-allocator and
          must be converted to a typed allocator before it may be used.

  .. function::
    allocator(mlib_allocator a)

    Convert from an `mlib_allocator` `a`.

  .. function::
    template <typename U> allocator(allocator<U>)

    Convert-construct from another allocator instance, rebinding the allocated
    type.

  .. function:: bool operator==(allocator) const

    Compare two allocators. Two `allocator`\ s are equal if the
    `mlib_allocator::userdata` and `mlib_allocator::reallocate` pointers
    are equal.

  .. function:: mlib_allocator c_allocator() const

    Obtain the `mlib_allocator` that is used by this `allocator`

  .. function::
    pointer allocate(std::size_t n) const
    void deallocate(pointer p, std::size_t n) const

    The allocation/deallocation functions for the C++ allocator interface.

    :param n: The number of objects to be allocated/deallocated
    :param p: Pointer to a previous region obtained from an equivalent `allocator`

    Calls `mlib_allocate`/`mlib_deallocate` to perform the allocation.

  .. function::
    template <typename... Args> \
    pointer new_(Args&&...) const
    void delete_(pointer p) const

    New/delete individual objects using the allocator.

  .. function::
    template <typename U> allocator<U> rebind() const

    Rebind the type parameter for the allocator.

  .. function::
    template <typename... Args> \
    void construct(pointer p, Args&&... args) const

    Construct an object at `p` with `uses-allocator construction`__. This will
    "inject" the allocator into objects that support construction using the same
    memory allocator. This allows the following to work properly::

      // An allocator to be used
      mlib::allocator<> a = get_some_allocator();
      // A string type that uses an mlib allocator
      using string = std::basic_string<char, std::char_traits<char>, mlib::allocator<char>>
      // Construct a vector with our allocator
      std::vector<string, mlib::allocator<string>> strings{a};
      // Append a new string
      strings.emplace_back("hello, world!");  // [note]

    On the line marked ``[note]`` we are emplace-constructing a string from a
    character array. This would not work if the `construct` method was not
    available, as the vector would try to default-construct a new
    `mlib::allocator`, which is not allowed. In this example, `emplace_back`
    will end up calling `allocator::construct` for the string, which will
    inject the parent allocator into the string during construction.

    __ https://en.cppreference.com/w/cpp/memory/uses_allocator


.. class::
  template <typename Alloc, typename T> \
  bind_allocator

  Create an object with a bound allocator.

  :header: |this-header|

  This class type supports CTAD, and using CTAD is recommended. It will
  perfect-forward the bound object into the resulting `bind_allocator` wrapper.

  .. function::
    bind_allocator(Alloc a, T&& obj)

    Bind the allocator `a` to the object `obj`

  .. type:: allocator_type = Alloc
  .. function:: allocator_type get_allocator() const

    Return the allocate that was bound with this object

  .. function::
    decltype(auto) operator()(auto&&...)

    Call the underlying invocable with the given arguments, if such a call is
    well-formed.

    This method is cvref-overloaded for the underlying object.

  .. function::
    auto query(auto q) const

    Apply a query to the underlying object. (See: :doc:`/dev/queries`)


.. struct:: alloc_deleter

  A deleter type for use with `std::unique_ptr` that deletes an object using an
  `mlib::allocator`

  :header: |this-header|

.. type::
  template <typename T> unique_ptr = std::unique_ptr<T, alloc_deleter>

  A `std::unique_ptr` type that uses an `mlib::allocator`.

  :header: |this-header|

  .. seealso:: `allocate_unique`


Functions
*********

.. function::
  template <typename T> \
  unique_ptr<T> allocate_unique(allocator<> a, auto&&... args)

  Construct an `mlib::unique_ptr\<T>` using the given allocator to manage the
  object.

  :header: |this-header|


Constants
*********

.. var:: const allocator<> terminating_allocator{::amongoc_terminating_allocator}

  A C++ version of the `amongoc_terminating_allocator`

  :header: |this-header|
