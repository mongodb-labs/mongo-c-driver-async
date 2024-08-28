####################################
Header: :header-file:`amongoc/box.h`
####################################

.. header-file:: amongoc/box.h

  This header defines types and functions for dealing with `amongoc_box`, a
  generic container of arbitrary values.

.. struct:: amongoc_box

  A type-erased boxed value of arbitrary type. The box manages storage for the
  boxed value and may contain a destructor function that will be executed when
  the box is destroyed.

  When a box parameter or return value is documented with |attr.type|, this
  specifies the type that is expected to be contained within the box. The
  special type ``[[type(nil)]]`` refers to a the `amongoc_nil` special box
  value.

  In C code, use :c:macro:`amongoc_box_init` to initialize box objects.

  When working with C++ code, prefer to use `amongoc::unique_box`, as it will
  prevent accidental copying and ensures the destructor is executed on the box
  when it goes out of scope.

  .. rubric:: Members

  All `amongoc_box` data members other than `view` are private and should not be
  accessed or modified.

  .. member:: amongoc_view view

    Accessing this member will yield an `amongoc_view` that refers to this box's
    contents.

    .. note:: It is only legal to access this member if :ref:`the box is active <box.active>`

  .. function:: as_unique() &&

    |C++ API| Move the box into an `amongoc::unique_box` to be managed automatically.


.. struct:: amongoc_view

  A read-only view of an `amongoc_box` value.

  An `amongoc_view` :math:`V` is *valid* if it was created by accessing the
  `amongoc_box::view` member of an :ref:`active <box.active>` `amongoc_box`
  :math:`B`, and is only valid as long the original :math:`B` remains active.
  :ref:`Consuming <box.consume>` a box :math:`B` will invalidate all
  `amongoc_view` objects that were created from :math:`B`.

  .. function:: template <typename T> T& as() noexcept

    The same as `amongoc::unique_box::as`


.. rubric:: Namespace: ``amongoc``
.. namespace:: amongoc
.. type:: box = ::amongoc_box

  `amongoc::box` is a type alias of `::amongoc_box`

.. class:: unique_box

  |C++ API| Wraps an `amongoc_box`, restricting copying and ensuring destruction to
  prevent programmer error. The `unique_box` is move-only.

  .. function:: unique_box(amongoc_box&&)

    Take ownership of the given box. The box must be passed as an
    rvalue-reference to emphasize this ownership transfer. The moved-from box
    will be overwritten with `amongoc_nil`.

  .. function:: ~unique_box()

    Destroy the underlying box.

  .. function:: operator amongoc_view()

    Implicit conversion to an `amongoc_view`

  .. function:: template <typename T> T& as() noexcept

    Obtain an l-value reference to the contained value of type `T`.

    :precondition: The :ref:`box must be active <box.active>` for the type `T`.
    :c API: :c:macro:`amongoc_box_cast`

  .. function::
    template <typename T> \
    static unique_box from(cxx_allocator<>, T&& value)

    Construct a new `unique_box` by decay-copying from the given value. This
    should be the preferred way to create box objects within C++ code.

    :throw std::bad_alloc: If memory allocation fails. This will never throw
      if the box is :ref:`small <box.small>`.
    :postcondition: The returned box object is :ref:`active <box.active>` for
      the decayed type of `T`.

  .. function::
    template <typename T, typename D> \
    static unique_box from(cxx_allocator<>, T value, D) \
    requires std::is_trivially_destructible_v<T>

    Create a new box object by copying the given value and imbuing it with a
    destructor based on `D`. The type `T` must be trivially destructible,
    because the box will instead use `D` as a destructor.

    In general, the given destructor should be a stateless function-object type
    (e.g. a lambda expression with no captures) that accepts a ``T&`` and
    destroys the object. Using anything else (e.g. a function pointer) will
    not work.

    :throw std::bad_alloc: If memory allocation fails. This will never throw
      if the box is :ref:`small <box.small>`.
    :postcondition: The returned box object is :ref:`active <box.active>` for
      the type `T`.

  .. function::
    template <typename T, typename... Args> \
    static unique_box make(cxx_allocator<> a, Args&&... args)

    In-place construct a new instance of `T` into a new box.

    :param a: The allocator to be used for the box.
    :param args: Constructor arguments for the new `T`.

  .. function:: [[nodiscard]] amongoc_box release() && noexcept

    Relinquish ownership of the underlying box and return it to the caller. This
    function is used to interface with C APIs that will |attr.transfer| an
    `amongoc_box` by-value.


.. Reset to the global namespace
.. namespace:: 0


Box Behavior
############

At any given time, an `amongoc_box` is either *active* for type T, or *dead*.


.. _box.active:

State: Active for type ``T``
****************************

A box :math:`B` is *active* for type ``T`` if **either**:

- :math:`B` was used with
  :c:macro:`amongoc_box_init`/:c:macro:`amongoc_box_init_noinline` with the
  type ``T``
- **OR** :math:`B` was created with a C++ API that constructs a box,
- **OR** :math:`B` is a by-value copy of an `amongoc_box` that was already
  active for type ``T``.

**AND**:

- :math:`B` has not been *consumed* by any operation.

If a box is active for type ``T``, then it is legal to use it in
:c:macro:`amongoc_box_cast` with type ``T``.


.. _box.dead:

State: Dead
***********

A box :math:`B` is *dead* if either:

- :math:`B` is newly declared and uninitialized.
- **or** :math:`B` was used in any operation that *consumed* it.


.. _box.consume:

Consuming Operations
********************

A box :math:`B` is *consumed* by any of the following operations:

- Passing :math:`B` by-value to any function parameter marked with
  |attr.transfer|.
- Returning :math:`B` by-value from a function.
- Copy-assigning :math:`B` into another l-value expression of type `amongoc_box`.


Relocation
**********

The `amongoc_box` should be considered *trivially relocatable*. That is: A
byte-wise copy of the object *can* be considered a moved-to `amongoc_box`,
invalidating the box that was copied-from (i.e.
:ref:`consuming it <box.consume>`).


.. _box.small:

Smallness
*********

`amongoc_box` considers some objects to be "small". If those objects are small,
then it is guaranteed that `amongoc_box` will not allocate memory for storing
those objects.

The only types **guaranteed** to be considered "small" are objects no larger
than two pointers.


Non-Relocatable Types
*********************

To store an object that cannot be trivially relocated within an `amongoc_box`,
one should use :c:macro:`amongoc_box_init_noinline`, which forcibly disables
the small-object optimization within the created box.

The C++ APIs `amongoc::unique_box::from` will automatically handle this
distiction by consulting `amongoc::enable_trivially_relocatable`.


.. _box.trivial:

Triviallity
***********

An `amongoc_box` is said to be *trivial* if the type it contains is
:ref:`small <box.small>` and the box has no associated destructor.

When a box is *trivial*, some usage requirements relax:

1. A trivial box may be copied arbitrarily without invalidating other copies,
   and each copy has a distinct identity.
2. It is safe to discard a trivial box (allow it to leave scope) without ever
   calling `amongoc_box_destroy`.
3. It is safe to overwrite or reinitialize the box (e.g.
   :c:macro:`amongoc_box_init`) with a new value without first destroying the
   box.

In general: the semantics of the |attr.transfer| attribute do not apply to
trivial boxes.

.. note::

  It is not sufficient that the box is simply small or contains a primitive
  type: It is possible that such a box has a destructor that needs to execute on
  the primitive's value (e.g. POSIX ``close`` is a destructor for an ``int``).


Other
#####

.. c:macro::
    amongoc_box_init(Box, T, ...)
    amongoc_box_init_noinline(Box, T, ...)

  Initialize a box to contain a zero-initialized storage for an instance of ``T``.

  :C++ API:
    - `amongoc::unique_box::from`
  :param Box: An non-const lvalue expression of type `amongoc_box`. This is the
    box that will be initiatlized.
  :param T: The type that should be stored within the box.
  :param Dtor: (Optional) A destructor function that should be executed when
    the box is destroyed with `amongoc_box_destroy`. The destructor function
    should be convertible to a function pointer: :cpp:any:`amongoc_box_destructor`
  :param Alloc: (Optional) An `amongoc_allocator` object to be used if the box
    requires dynamic allocation.
  :return: This macro will result in a :cpp:`T*` pointer. If memory allocation
    was required and fails, this returns :cpp:`nullptr`. Note that a
    :ref:`small <box.small>` type will never fail to allocate, so the returned
    pointer to a small object will never be null.

  The ``_noinline`` variant of this macro inhibits the small-object
  optimization, which is required if the object being stored is not relocatable
  (i.e. it must be address-stable).

  .. note::

    The given box must be either :ref:`dead <box.dead>` or
    :ref:`trivial <box.trivial>`, or the behavior is undefined.


.. c:macro:: amongoc_box_cast(T)

  :param T: The target type for the cast expression.
  :C++ API: `amongoc::unique_box::as` and `amongoc_view::as`

  Perform a cast from an :cpp:any:`amongoc_box` or :cpp:any:`amongoc_view` to an
  l-value expression of type ``T``. :c:expr:`amongoc_box_cast(...)` is only a
  prefix to the full cast, which must be passed a box within another set of
  parentheses::

    void handle_boxed_int(amongoc_box b) {
      // Copy an `int` from the box
      int n = amongoc_box_cast(int)(b);
    }

  Note that because the result is an l-value expression, this cast expression
  can be used to manipulate the value stored in the box::

    void changed_boxed_int(amongoc_box* b) {
      // Replace the boxed integer value with 42
      amongoc_box_cast(int)(*b) = 42;
    }

  If the given box is not active for the type ``T``, then the behavior is
  undefined.


.. c:macro:: amongoc_box_take(Dest, Box)

  :param Dest: An l-value expression of type :math:`T` that will receive the
    boxed value.
  :param Box: |attr.transfer| A box that is :ref:`active <box.active>` for the type :math:`T`.

  Moves the value stored in ``Box`` to overwrite the object ``Dest``.

  This is useful to move an object from the type-erased box into a typed storage
  variable for more convenient access. The dynamic storage for ``Box`` will be
  released, but the destructor for the box will not be executed. The object is
  now stored within ``Dest`` and it is up to the caller to manage its lifetime.

  .. rubric:: Example

  ::

    struct my_large_object {
      int values[64];
    };

    // ...
    void foo(amongoc_box large) {
      my_large_object o;
      amongoc_box_take(o, large);
      // `o` now has the value from `large`, and dynamic storage for `large`
      // has been released.
    }


.. function:: void amongoc_box_destroy(amongoc_box [[transfer]] b)

  Consume the given box and destroy its contents.

  :param b: The box that will be consumed and whose contained value will be
    destroyed.


.. type:: amongoc_box_destructor = void(*)(void* p)

  Type of the destructor function that may be associated with a box. The
  function parameter ``p`` is a pointer to the object that was stored within
  the box.

  After the destructor function is invoked, any dynamic storage associated with
  the box will be released.


.. function:: void amongoc_box_free_storage(amongoc_box [[transfer]] b)

  .. note:: Do not confuse this with `amongoc_box_destroy`

  This function will release dynamically allocated storage associated with the
  given box without destroying the value that it may have contained.

  This function should be used when the value within the box is moved-from, and
  the box itself is no longer needed.


.. var:: constexpr amongoc_box amongoc_nil

  A box value that contains no value. The resulting `amongoc_box` is
  :ref:`trivial <box.trivial>`. Destroying a box constructed from `amongoc_nil`
  is a no-op.

  .. note:: For C compatibility, this is actually implemented as a macro, not a variable.

.. function::
  amongoc_box amongoc_box_pointer(const void* x)
  amongoc_box amongoc_box_float(float x)
  amongoc_box amongoc_box_double(double x)
  amongoc_box amongoc_box_char(char x)
  amongoc_box amongoc_box_short(short x)
  amongoc_box amongoc_box_int(int x)
  amongoc_box amongoc_box_unsigned(unsigned int x)
  amongoc_box amongoc_box_long(long x)
  amongoc_box amongoc_box_ulong(unsigned long x)
  amongoc_box amongoc_box_longlong(long long x)
  amongoc_box amongoc_box_ulonglong(unsigned long long x)
  amongoc_box amongoc_box_size(std::size_t x)
  amongoc_box amongoc_box_ptrdiff(std::ptrdiff_t x)
  amongoc_box amongoc_box_int8(std::int8_t x)
  amongoc_box amongoc_box_uint8(std::uint8_t x)
  amongoc_box amongoc_box_int16(std::int16_t x)
  amongoc_box amongoc_box_uint16(std::uint16_t x)
  amongoc_box amongoc_box_int32(std::int32_t x)
  amongoc_box amongoc_box_uint32(std::uint32_t x)
  amongoc_box amongoc_box_int64(std::int64_t x)
  amongoc_box amongoc_box_uint64(std::uint64_t x)

  Convenience functions that initialize a new `amongoc_box` with the type and
  value of the given argument.

  Note that all of the boxes returned by these functions are
  :ref:`trivial <box.trivial>`.

.. namespace:: amongoc

.. var:: template <typename T> constexpr bool enable_trivially_relocatable

  Trait variable template that determines whether `amongoc::unique_box::from`
  will try to store an object inline within a box (omitting allocation).

  By default any objects that are both trivially destructible and trivially
  move-constructible are considered to be trivially relocatable.

  By the above rule all of the following are considered trivially relocatable:

  - All built-in language types
  - All pointer types
  - All class types that have no non-trivial move/destroy operations (including
    all pure C structs)
  - C++ closure objects that have no non-trivial move/destroy operations (this
    is based on the type of values that it captures).

  Additionally, if the type `T` has a nested static member
  ``enable_trivially_relocatable`` that is truth-y, then the object will be
  treated as trivially relocatable.
