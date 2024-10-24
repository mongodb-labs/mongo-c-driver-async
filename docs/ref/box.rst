####################################
Header: :header-file:`amongoc/box.h`
####################################

.. header-file:: amongoc/box.h

  This header defines types and functions for dealing with `amongoc_box`, a
  generic container of arbitrary values.

.. |this-header| replace:: :header-file:`amongoc/box.h`

Types
#####

.. struct:: [[zero_initializable]] amongoc_box

  A type-erased boxed value of arbitrary type. The box manages storage for the
  boxed value and may contain a destructor function that will be executed when
  the box is destroyed.

  :zero-initialized: |attr.zero-init| A zero-initialized `amongoc_box` is
    equivalent to `amongoc_nil`.

  :header: |this-header|

  When a box parameter or return value is documented with |attr.type|, this
  specifies the type that is expected to be contained within the box. The
  special type ``[[type(nil)]]`` refers to a the `amongoc_nil` special box
  value.

  In C code, use `amongoc_box_init` to initialize box objects.

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

  :header: |this-header|

  An `amongoc_view` :math:`V` is *valid* if it was created by accessing the
  `amongoc_box::view` member of an :ref:`active <box.active>` `amongoc_box`
  |B|, and is only valid as long the original |B| remains active.
  :ref:`Consuming <box.consume>` a box |B| will invalidate all
  `amongoc_view` objects that were created from |B|.

  .. function:: template <typename T> T& as() noexcept

    The same as `amongoc::unique_box::as`


.. type:: __box_or_view

  A special exposition-only parameter type representing either an `amongoc_box`
  or an `amongoc_view`.


.. type:: amongoc_box_destructor = void(*)(void* p)

  Type of the destructor function that may be associated with a box. The
  function parameter ``p`` is a pointer to the object that was stored within
  the box.

  After the destructor function is invoked, any dynamic storage associated with
  the box will be released.


.. type:: amongoc::box = ::amongoc_box

  `amongoc::box` is a type alias of `::amongoc_box`

  :header: |this-header|


.. class:: amongoc::unique_box

  |C++ API| Wraps an `amongoc_box`, restricting copying and ensuring destruction to
  prevent programmer error. The `unique_box` is move-only.

  :header: |this-header|

  .. note::

    `unique_box` is not default-constructible. If you want a reasonable
    "nothing" box, using `amongoc::nil` to initialize a new instance.

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
    :c API: `amongoc_box_cast`

  .. function::
    template <typename T> \
    static unique_box from(mlib::allocator<>, T&& value)

    Construct a new `unique_box` by decay-copying from the given value. This
    should be the preferred way to create box objects within C++ code.

    :throw std__bad_alloc: If memory allocation fails. This will never throw
      if the box is :ref:`small <box.small>`.
    :postcondition: The returned box object is :ref:`active <box.active>` for
      the decayed type of `T`.

  .. function::
    template <typename T, typename D> \
    static unique_box from(mlib::allocator<>, T value, D) \
    requires std::is_trivially_destructible_v<T>

    Create a new box object by copying the given value and imbuing it with a
    destructor based on `D`. The type `T` must be trivially destructible,
    because the box will instead use `D` as a destructor.

    In general, the given destructor should be a stateless function-object type
    (e.g. a lambda expression with no captures) that accepts a ``T&`` and
    destroys the object. Using anything else (e.g. a function pointer) will
    not work.

    :throw std__bad_alloc: If memory allocation fails. This will never throw
      if the box is :ref:`small <box.small>`.
    :postcondition: The returned box object is :ref:`active <box.active>` for
      the type `T`.

  .. function::
    template <typename T, typename... Args> \
    static unique_box make(mlib::allocator<> a, Args&&... args)

    In-place construct a new instance of `T` into a new box.

    :param a: The allocator to be used for the box.
    :param args: Constructor arguments for the new `T`.

  .. function:: [[nodiscard]] amongoc_box release() && noexcept

    Relinquish ownership of the underlying box and return it to the caller. This
    function is used to interface with C APIs that will |attr.transfer| an
    `amongoc_box` by-value.

  .. function::
    void* data();
    const void* data() const;

    Obtain a pointer to the data stored in the box.

    :C API: `amongoc_box_data`


Functions & Macros
##################

Box Creation / Destruction
**************************

.. function::
  amongoc_box_init(amongoc_box b, __type T)
  amongoc_box_init(amongoc_box b, __type T, amongoc_box_destructor dtor)
  amongoc_box_init(amongoc_box b, __type T, amongoc_box_destructor dtor, mlib_allocator alloc)
  amongoc_box_init_noinline(amongoc_box b, __type T)
  amongoc_box_init_noinline(amongoc_box b, __type T, amongoc_box_destructor dtor)
  amongoc_box_init_noinline(amongoc_box b, __type T, amongoc_box_destructor dtor, mlib_allocator alloc)

  Initialize a box to contain a |zero-initialized| storage for an instance of
  type `T`.

  :C++ API: `amongoc::unique_box::from`
  :param b: An non-const lvalue expression of type `amongoc_box`. This is the
    box that will be initiatlized.
  :param T: The type that should be stored within the box.
  :param dtor: A destructor function that should be executed when the box is
    destroyed with `amongoc_box_destroy`. The destructor function should be
    convertible to a function pointer: :cpp:any:`amongoc_box_destructor`. If
    omitted, the box will have no associated destructor.
  :param alloc: An `mlib_allocator` object to be used if the box requires
    dynamic allocation. If omitted, the default allocator will be used.
  :return: Returns a non-|const| pointer to `T`. If memory allocation was
    required and fails, this returns :cpp:`nullptr`. Note that a
    :ref:`small <box.small>` type used with `amongoc_box_init` will not
    allocate, so the returned pointer in such a scenario will never be null.

  The ``_noinline`` variant will inhibit the small-object optimization, which is
  required if the object being stored is not relocatable (i.e. it must be
  address-stable).

  .. note::

    The given box must be either :ref:`dead <box.dead>` or
    :ref:`trivial <box.trivial>`, or the behavior is undefined.


.. function:: void amongoc_box_destroy(amongoc_box [[transfer]] b)

  Consume the given box and destroy its contents.

  :param b: |attr.transfer| The box that will be consumed and whose contained
    value will be destroyed.


.. function:: void amongoc_box_free_storage(amongoc_box [[transfer]] b)

  .. note:: Do not confuse this with `amongoc_box_destroy`

  This function will release dynamically allocated storage associated with the
  given box without destroying the value that it may have contained.

  This function should be used when the value within the box is moved-from, and
  the box itself is no longer needed.


Inspection
**********

.. function:: T amongoc_box_cast(__type T, __box_or_view box)

  :param T: The target type for the cast expression.
  :C++ API: `amongoc::unique_box::as` and `amongoc_view::as`

  Perform a cast from an :cpp:any:`amongoc_box` or :cpp:any:`amongoc_view` to an
  l-value expression of type `T`.

  Note that because the result is an l-value expression, this cast expression
  can be used to manipulate the value stored in the box::

    void changed_boxed_int(amongoc_box* b) {
      // Replace the boxed integer value with 42
      amongoc_box_cast(int, *b) = 42;
    }

  If the given box is not active for the type `T`, then the behavior is
  undefined.


.. function::
  void* amongoc_box_data(amongoc_box b)
  const void* amongoc_box_data(const amongoc_box b)
  const void* amongoc_box_data(amongoc_view b)

  Obtain a pointer to the object stored within a box `b`. Expands to an r-value
  of type :cpp:`void*`. If `b` is a |const| box or an `amongoc_view`, the
  returned pointer is a pointer-to-|const|.

  :C++ API: `amongoc::unique_box::data`

  .. note:: |macro-impl|.


.. function:: void amongoc_box_take(auto dest, amongoc_box [[transfer]] box)

  Moves the value stored in `box` to overwrite the object `dest`.

  :param dest: A non-|const| l-value expression of type |T| that will receive
    the boxed value.
  :param box: |attr.transfer| A non-|const| box that is
    :ref:`active <box.active>` for the type |T|.

  This is useful to move an object from the type-erased box into a typed storage
  variable for more convenient access. The dynamic storage for `box` will be
  released, but the destructor for the box will not be executed. The object is
  now stored within `dest` and it is up to the caller to manage its lifetime.

  .. note:: |macro-impl|.

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


Trivial Box Constructors
************************

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
  amongoc_box amongoc_box_size(size_t x)
  amongoc_box amongoc_box_ptrdiff(ptrdiff_t x)
  amongoc_box amongoc_box_int8(int8_t x)
  amongoc_box amongoc_box_uint8(uint8_t x)
  amongoc_box amongoc_box_int16(int16_t x)
  amongoc_box amongoc_box_uint16(uint16_t x)
  amongoc_box amongoc_box_int32(int32_t x)
  amongoc_box amongoc_box_uint32(uint32_t x)
  amongoc_box amongoc_box_int64(int64_t x)
  amongoc_box amongoc_box_uint64(uint64_t x)

  Convenience functions that initialize a new `amongoc_box` with the type and
  value of the given argument.

  Note that all of the boxes returned by these functions are
  :ref:`trivial <box.trivial>`.


.. function:: unique_box amongoc::nil() noexcept

  Returns a unique box containing no value.

  :C API: `amongoc_nil`


Constants
#########

.. var:: const amongoc_box amongoc_nil

  A box value that contains no value. The resulting `amongoc_box` is
  :ref:`trivial <box.trivial>`. Destroying a box constructed from `amongoc_nil`
  is a no-op.

  :C++ API: `amongoc::nil`

  .. note:: |macro-impl|.


.. var:: template <typename T> constexpr bool amongoc::enable_trivially_relocatable_v

  Trait variable template that determines whether `amongoc::unique_box::from`
  will try to store an object inline within a box (omitting allocation).

  :header: ``amongoc/relocation.hpp``

  By default any objects that are both trivially destructible and trivially
  move-constructible are considered to be trivially relocatable.

  By the above rule all of the following are considered trivially relocatable:

  - All built-in language types
  - All pointer types
  - All class types that have no non-trivial move/destroy operations (including
    all pure C structs)
  - C++ closure objects that have no non-trivial move/destroy operations (this
    is based on the type of values that it captures).

  Additionally, if the type `T` has a nested type
  ``enable_trivially_relocatable`` that is defined to `T`, then the object will
  be treated as trivially relocatable.


Box Behavior
############

At any given time, an `amongoc_box` is either *active* for type T, or *dead*.


.. _box.active:

State: Active for type |T|
**************************

A box |B| is *active* for type |T| if **either**:

- |B| was used with `amongoc_box_init`/`amongoc_box_init_noinline` with the type
  |T|
- **OR** |B| was created with a C++ API that constructs a box,
- **OR** |B| is a by-value copy of an `amongoc_box` that was already
  active for type |T|.

**AND**:

- |B| has not been *consumed* by any operation (i.e. passed through a
  |attr.transfer| parameter)

If a box is active for type |T|, then it is legal to use it in
`amongoc_box_cast` with type |T|.


.. _box.dead:

State: Dead
***********

A box |B| is *dead* if either:

- |B| is newly declared and uninitialized.
- **or** |B| was used in any operation that *consumed* it.


.. _box.consume:

Consuming Operations
********************

A box |B| is *consumed* by any of the following operations:

- Passing |B| by-value to any function parameter marked with
  |attr.transfer|.
- Returning |B| by-value from a function.
- Copy-assigning |B| into another l-value expression of type `amongoc_box`.


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
one should use `amongoc_box_init_noinline`, which forcibly disables the
small-object optimization within the created box.

The C++ APIs `amongoc::unique_box::from` will automatically handle this
distiction by consulting `amongoc::enable_trivially_relocatable_v`.


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
3. It is safe to overwrite or reinitialize the box (e.g. `amongoc_box_init`)
   with a new value without first destroying the box.

In general: the semantics of the |attr.transfer| attribute do not apply to
trivial boxes.

.. note::

  It is not sufficient that the box is simply small or contains a primitive
  type: It is possible that such a box has a destructor that needs to execute on
  the primitive's value (e.g. POSIX ``close`` is a destructor for an ``int``).


Storage Alignment
*****************

.. important::

  At the current time, boxes allocate and store values using the default
  maximum-alignment defined by the compiler. There is not yet support for types
  that require additional alignment.


Q: "Can I query the state of a box?"
************************************

In general, *no*. The properties of a box (i.e. type, state, triviallity,
smallness, whether it is nil, and whether it has a destructor) are stored as
implementation details. **Code should be designed to treat all live boxes as
non-trivial** unless they are known to be otherwise.

Attributes of boxes may be carried in other channels, e.g through an associated
`amongoc_status` parameter, but it is up to the particular box+status pair to
define the semantics thereof.
