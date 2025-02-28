########################################
Meta-Reference: Documentation Attributes
########################################

Some APIs in this documentation contain non-standard language attributes on
functions, parameters, types, and variables. These only appear in the
documentation and are not part of the actual C API headers. They do not effect
the actual compilation or behavior of the program, and are intended to act as
common shorthands for behavioral guarantees of the annotated APIs.


Non-Standard Attributes
#######################

|attr.transfer|
***************

.. doc-attr:: [[transfer]]

  Understanding the meaning of |attr.transfer| is essential to understanding
  object lifetimes in |amongoc|. The |attr.transfer| has two significant points:

  1. If a function parameter is annotated with |attr.transfer|, it means that
     passing a value to that function will invalidate the value in the caller's
     scope.
  2. When an object is passed through a parameter that is marked with
     |attr.transfer|, it becomes the responsibility of the called function to
     manage the lifetime of the associated object. The function **must**
     eventually pass the object to another function that has an |attr.transfer|
     attribute.\ [#box-trivial-note]_

  According to point **1**: If you pass an :term:`lvalue` to an object |V|
  through a parameter marked |attr.transfer|, then the |V| object is now
  *invalid*. See: :ref:`after-transfer`.

  According to point **2**: If you pass a function pointer to an API, and the
  function pointer type has |attr.transfer| parameters, then any value passed
  in through that parameter now *owns* the respective object, and it must
  handle it properly::

    // `call_through` transfers ownership of an object through `i`
    extern int call_through(void (*callback)(int [[transfer]] i));

    // For use with `call_through`
    int my_callback(int i) {
      // We own the `i` object!
    }

    int my_code() {
      call_through(&my_callback);
    }


Example: POSIX ``close``
========================

A correct use of |attr.transfer| would be on the ``close()`` POSIX API::

  int close(int [[transfer]] filedes);

Because we are passing ``filedes`` by-value to ``close()``, it is not possible
for ``close()`` to actually modify our copy of the file descriptor.
Nevertheless, ``close()`` will *invalidate* the file descriptor, even if our
copy is completely unmodified::

  int fd = get_fileno();  // [1]
  close(fd);              // [2]
  use_file(fd);           // [3]

Even though the ``fd`` value on ``[3]`` is bitwise-equal to the value it was
assigned on ``[1]``, it is inherently erroneous: The call to ``close()`` on
``[2]`` has made the bit pattern contained in ``fd`` invalid as a file
descriptor, and the behavior of line ``[3]`` is unspecified.\ [#ebadf]_

Because ``close()`` "poisons" the bit-pattern of the passed file descriptor, it
would be correct to annotate the parameter with |attr.transfer|.


Example: C ``free``
===================

The C function ``free`` could also be annotated as |attr.transfer| on the passed
pointer value::

  void free(void* [[transfer]] p)

This follows similar behavior to ``close()``: Even though the bits of the
pointer are unmodified in the calling scope, the pointer's value itself has
become poisoned and should not be used for any subsequent operation.


Example: `amongoc_box_destroy`
==============================

The `amongoc_box_destroy` function takes an `amongoc_box` by-value, and its parameter
is annotated with |attr.transfer|.

Like with ``close()`` and ``free()``, the caller's copy of the passed value is
unmodifed, but is still invalidated. Like with ``close()`` and ``free()``,
subsequent uses of the object are unspecified/undefined, including attempting to
destroy the box a second time::

  amongoc_box got_box = create_a_box();
  amongoc_box_destroy(got_box);         // Okay
  amongoc_box_destroy(got_box);         // UNDEFINED BEHAVIOR


.. _after-transfer:

Dealing with Transferred Objects
================================

After passing an object to a |attr.transfer| function parameter, the only legal\
[#box-trivial-note]_ operations on that object are reassignment (replacing the
object with a live value) or discarding (letting the object leave scope).


|attr.nullable|
***************

.. doc-attr:: [[nullable]]

  A parameter marked with |attr.nullable| indicates that passing a null-value
  for the object is valid. For pointers, this indicates that passing
  :cpp:`NULL`/:cpp:`nullptr` has well-defined behavior. Other pointer-like
  objects may also have a null state, usually their |zero-initialized| state.

  If a parameter is not marked |attr.nullable|, assume that passing a null
  argument will introduce undefined behavior.


|attr.zero-init|
****************

.. doc-attr:: [[zero_initializable]]

  When an aggregate type is annotated with |attr.zero-init|, it indicates that a
  zero-initialized/empty-initialized instance of that aggregate type is a valid
  object for certain operations. Usually, this corresponds to a null or zero
  state. A type that is |attr.zero-init| should have a **Zero-initialized**
  field describing the semantics of such a value.

  For such an object, declaring a |static| instance with no explicit initializer
  will be valid, as well as initializing using empty braces :cpp:`{}`, or using
  `memset` to fill its object representation with zero-bytes.


.. _zero-init:

Zero Initialization / Empty Initialization
==========================================

`Zero initialization`__ (C++) and `empty initialization`__ (C) are similar
concepts with similar behavior. For convenience, this documentation refers to
*zero initialization* to mean either the C++ concept or *empty initialization*
in C.

In C++ and C23, a trivial aggregate may be initialized with an empty brace pair
:cpp:`{ }` to achieve empt/zero-initialization (this is called *value
initialization* in C++). In prior C versions, initializing with a brace pair and
a single literal zero :cpp:`{ 0 }` will usually achieve the same effect. Some C
compilers implement the C23 language feature as an extension in earlier C
versions. An object declared |static| without an explicit initializer will
always be zero-initialized at compile time.

__ https://en.cppreference.com/w/cpp/language/zero_initialization
__ https://en.cppreference.com/w/c/language/initialization


|attr.type|
***********

.. doc-attr:: [[type(T)]]

  Associates a type with a type-erased object. The meaning of this association
  depends on the object container type. The following types are often used with
  |attr.type|:

  - `amongoc_emitter` - Specifies the success result type of the emitter.
  - `amongoc_handler` - Specifies the result type expected by the handler.
  - `amongoc_box` and `amongoc_view` - Specifies the type that is contained
    within the box for use with `amongoc_box_cast`
  - ``void*`` - Specifies the pointed-to type for the pointer.


|attr.storage|
**************

.. doc-attr:: [[storage]]

  When attached to a pointer parameter, this attribute indicates that the
  pointer is treated as uninitialized storage for an object of the appropriate
  type. The API using |attr.storage| will not attempt to destroy or read from
  the pointed-to location.


:doc-attr:`[[optional]]`
************************

.. doc-attr:: [[optional]]

  When applied to a method declaration in a virtual method table, indicates that
  the associated method pointer may be ``NULL``.

  For any methods not declared with :doc-attr:`[[optional]]`, assume that the
  method is required.


Parameter Types
###############

`__type` Parameters
*******************

.. type:: __type

  Certain :term:`function-like macro`\ s are documented as functions, and they
  may be annotated with a `__type` parameter. This indicates that the
  corresponding macro argument should be a compile-time type specifier rather
  than a runtime value.


Unspecified Types
*****************

.. type:: __unspecified

  This documentation type indicates a private type that is not part of the
  public API, although an annotated struct field may be part of the public API.

  Dereferencing a pointer-to or accessing the members of an `__unspecified` type
  is not guaranteed to have well-defined behavior. A pointer-to-`__unspecified`
  should be considered a stronger-typed |void|.


Documentation Fields
####################

Field: **Allocation**
*********************

Some functions are annotated with an **Allocation** documentation field. This
specifies how the function will allocate dynamic memory. If this field is
ommitted, either the function does not allocate, uses the standard library
allocator, or it accepts an `mlib_allocator` directly.


Field: **Zero-initialized**
***************************

The **Zero-initialized** field appears on some data types and specifies the
semantics of that type when it has been
:ref:`zero/empty initialized <zero-init>`, if any.

If a type does not have such a field, assume that it has no defined semantics
when zero-initialized.


.. rubric:: Footnotes

.. [#ebadf] While using a closed file descriptor *may* result in ``EBADF``, it
  is entirely possible that a subsequent operation between ``close()`` and using
  the file descriptor (possibly on another thread) has caused the operating
  system to re-use the particular integer value of that file descriptor, and the
  behavior of the program becomes completely unpredictable.

.. [#box-trivial-note]

  There is an exemption to the rules of |attr.transfer| for objects that are
  "trivial". These exemptions are noted in the documentation in which they are
  relevant. In particular, an `amongoc_box` may be :ref:`trivial <box.trivial>`,
  meaning that it has no associated destructor nor dynamically allocated
  storage. These boxes may be freely copied and discarded even when used with
  |attr.transfer| parameters.
