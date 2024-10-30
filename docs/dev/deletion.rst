#################
C Object Deletion
#################

.. warning:: |devdocs-page|

|amongoc| contains several types, templates, and macros that aide in the
automatic deletion of C objects. All objects should follow the given rules in
order to ensure uniform deletion behavior across the codebase.

.. header-file:: mlib/delete.h

  This is a C-compatible header that is mostly useful for C++ code only.


Defining Deletion Methods
#########################

1. Any public C type that acts as an owner of resources, whether it is a pointer
   type or an aggregate type that contains resources, should have a
   specialization of `mlib::unique_deleter`. **Note** that explicit
   specialization is not usually necessary, and it will be done automatically
   when certain macros are used. Read below.
2. A type that is deletable using `mlib::unique_deleter` meets the
   `mlib::unique_deletable` concept.
3. For types which have subtle deletion behavior that is implemented as a
   hand-written C API function, use :c:macro:`mlib_assoc_deleter` to
   associated the C API function with a `mlib::unique_deleter` specialization.
4. For a simple struct type that contains resource-owning members which are
   themselves `mlib::unique_deletable`, use
   :c:macro:`mlib_declare_member_deleter` and use
   :c:macro:`mlib_declare_c_deletion_function`.
5. **Important**: Ensure that trying to delete on a |zero-initialized| value of
   a type is well-defined (a-la calling :cpp:`delete` on a null pointer, which
   is a safe no-op).


Deletion APIs
#############


.. function:: void mlib::delete_unique(auto& inst)

  Delete the object `inst` using the `unique_deleter` mechanism. This function
  is only callable if such a specialization of `unique_deleter` exists.

  :header: :header-file:`mlib/delete.h`

  .. tip:: This is implemented as a stateless invocable object.


.. concept:: template <typename T> mlib::unique_deletable

  Matches any type that can be passed to `delete_unique`


.. struct:: template <typename T> mlib::unique_deleter

  This struct template is used to determine how to automatically delete C objects
  of type `T`, including pointers and aggregates.

  :header: :header-file:`mlib/delete.h`

  This should *only* be used
  with types that are themselves trivially destructible. To delete a C object
  generically, use `mlib::delete_unique`. Any specialization of `unique_deleter`
  is treated as a stateless invocable object which must provide an `operator()`

  .. function:: void operator()(T& inst) const noexcept

    Any specialization of `unique_deleter` must have a function call operation
    that performs the actual deletion operation.


.. struct::
  template <typename T> \
  mlib::unique_deleter<T> : T::deleter

  :requires: :expr:`typename T::deleter`
  :header: :header-file:`mlib/delete.h`

  This specialization of `unique_deleter` will be used if the type `T` contains
  a nested type ``deleter``. That deleter will be used for deleting `T` objects.

  .. tip:: This is intended for use with :c:macro:`mlib_declare_member_deleter`


.. struct:: template <auto... MemPointers> mlib::delete_members

  This struct template creates a deletion invocable that deletes the members of
  a struct.

  :tparam MemPointers: Zero or more pointers-to-member-objects.
  :header: :header-file:`mlib/delete.h`

  When this deletion object is invoked on an instance of a type, for each member
  in `MemPointers`, `mlib::delete_unique` will be invoked on that instance's
  member in the listed order.

  .. tip:: Instead of using this directly, generate a specialization of it
    using the :c:macro:`mlib_declare_member_deleter` macro within a struct body.


.. c:macro:: mlib_declare_member_deleter(...)

  This variadic macro should appear within the body of a C struct, and each
  macro argument should be a pointer to a member of that struct.

  :header: :header-file:`mlib/delete.h`

  When compiled as C, this expands to an empty declaration.

  When compiled as C++, this expands to a nested typedef `deleter` which is a
  specialization of `mlib::delete_members`. This will notify the
  `mlib::unique_deleter` mechanism that deletion of the object should
  recursively delete the identified struct members.

  .. important:: Don't use this with :c:macro:`mlib_assoc_deleter`


.. c:macro:: mlib_declare_c_deletion_function(FuncName, Type)

  Declares a C-linkage function named by ``FuncName`` that accepts a ``Type``
  by-value. The body of that function will call `mlib::delete_unique` with the
  instance of the value.

  .. important:: Don't use this with :c:macro:`mlib_assoc_deleter`


.. c:macro:: mlib_assoc_deleter(Type, DeletionFunc)

  Creates a compile-time association between the given type and a C API deletion
  function. The function must be invocable with a modifiable l-value of type
  ``Type``.

  :header: :header-file:`mlib/delete.h`

  When compiled as C, this expands to an empty declaration.

  When compiled as C++, this expands to an explicit specialization of
  `mlib::unique_deleter` for the type ``Type``, which will invoke
  ``DeletionFunc`` for that type.

  .. important:: Don't use this with :c:macro:`mlib_declare_c_deletion_function`


Examples
########

Creating a simple deletable aggregate
*************************************

If I have an aggregate type that I want to make deletable using the
`mlib::unique_deleter` mechanism, that can be done as follows::

  #include <mlib/delete.h>

  typedef struct user_info {
    mlib_str username;
    mlib_str domain;
    int      uid;
    // Declare the deletion mechanism:
    mlib_declare_member_deleter(&user_info::username,
                                &user_info::domain);
  } user_info;

  // Declare a C API function that invokes mlib::delete_unique
  mlib_declare_c_deletion_function(user_info_delete, user_info);

This is C-compatible header content that declares a zero-initializable struct
``user_info``, along with a C API function ``user_info_delete`` which takes an
instance of ``user_info`` by-value and deletes the members of that object.


Creating a Custom Deletable Objects
***********************************

This example creates a type which is not simple to destroy, but we can still
register it with the `mlib::unique_deleter` mechanism::

  #include <mlib/delete.h>
  #include <mlib/alloc.h>

  typedef struct buncha_numbers {
    int*           integers;
    mlib_allocator alloc;
    size_t         n_numbers;
  } buncha_numbers;

  mlib_extern_c inline void buncha_numbers_delete(buncha_numbers n) {
    mlib_deallocate(n.alloc, n.integers, sizeof(int) * n.n_numbers);
  }

  // Associate our deletion function with unique_deleter
  mlib_assoc_deleter(buncha_numbers, buncha_numbers_delete);

This is a C-compatible interface that declares a type ``buncha_numbers`` and has
a C function ``buncha_numbers_delete``. Because of the API guarantees of
`mlib_deallocate`, it is safe to call with a zero-initialized allocator as long
as the pointer argument is also null.

The expansion of :c:macro:`mlib_assoc_deleter` will associate the C function
``buncha_numbers_delete`` with a `mlib::unique_deleter` specialization for the
type ``buncha_numbers``


Automatic Unique Objects
########################


.. warning:: |devdocs-page|

.. header-file:: mlib/unique.hpp

  Declares the `mlib::unique` class template


.. class:: template <typename T, typename Del = unique_deleter<T>> mlib::unique

  Automatically takes ownership of instances of `T` using the given deleter.

  Use with CTAD is supported.

  .. rubric:: Example

  ::

    extern "C" mlib_str makes_a_string();
    extern "C" void     takes_a_string(mlib_str s);

    void cxx_function() {
      // The string returned by `makes_a_string` will be automatically destroyed
      auto s1 = mlib::unique(makes_a_string());
      // Alternatively, with implicit conversion:
      mlib::unique s2 = makes_a_string();

      // We can pass the string along using release():
      takes_a_string(std::move(s2).release());
    }

  .. note::

    This type does not separately track whether is is moved-from or empty. It is
    up to the deleter to respect zero-initialized objects as being empty.


  .. function:: unique()

    Creates a |zero-initialized| instance of `T`

  .. function:: unique(T&& inst)

    Takes ownership of `inst`. `inst` will be reset to a |zero-initialized|
    value.

    .. note:: This is an *implicit conversion* constructor.

  .. function::
    unique(unique&& other)
    unique& operator=(unique&& other)

    Move from another unique instance. Destroys the currently held object and
    calls `release` on `other`

  .. function::
    [[nodiscard]] T release() &&

    Relinquish ownership of the held object. The current value is returned, and
    the held value is |zero-initialized|.

    This is r-value qualified to emphasize the ownership transfer.

  .. function::
    T& reset()
    T& reset(T&& value)

    Destroy the held value and replace it with either a zero-initialized
    instance or the given `value`.

  .. function::
    T* operator->()
    const T* operator->() const
    T& operator*()
    const T& operator*() const
    T& get()
    const T& get() const

    Obtain access to the wrapped object.
