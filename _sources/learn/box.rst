###################################
Storing Values in a Type-Erased Box
###################################

The `amongoc_box` type is used throughout |amongoc| for storing dynamically
typed values in a way that arbitrary code can execute a destructor on the box if
necessary. The box type is larger than a pointer, but is significantly more
efficient for storing small objects without requiring dynamic memory allocation.
An unknown box should be treated as an opaque pointer to an arbitrary object.


A Default Box
#############

The default "zero-initialized" value for a box is equivalent to `amongoc_nil`.
A declared box should either be zero-initialized or assigned from `amongoc_nil`:

.. literalinclude:: box.example.cpp
  :start-after: [uses-nil]
  :end-before: end.

Here, we constuct a box at the outer scope and initialize it to `amongoc_nil`.
In two different branches, we overwrite the nil box with something else and then
do work on that other value. Finally, we destroy the box at the end of the
function.

If neither branch is taken, then `amongoc_box_destroy` will be called on the
null box. This is a valid and safe operation that does nothing.

We are safe to overwrite an `amongoc_nil` box with another box only becaues the
box is guaranteed to be :ref:`trivial <box.trivial>`. If our box was
non-trivial, then we would need to destoy the box before we overwrite it with
another value.


Initializing a Box with a Value
###############################

To initialize the value stored in a box, use `amongoc_box_init`. Pass an
:term:`lvalue` expression referring to an `amongoc_box` as the first argument,
and a type as the second:

.. literalinclude:: box.example.cpp
  :start-after: [init-box-simple]
  :end-before: end.

`amongoc_box_init` will zero-initialize enough storage for the requested type
and returns a pointer to that storage. `amongoc_box_init` may or may not
dynamically allocate the object, depending on several factors.

.. note::

  In the above example, the box we create is :ref:`trivial <box.trivial>`,
  meaning that the call to `amongoc_box_destroy` is not actually necessary. In
  general, it is better to call `amongoc_box_destroy` unless you can be certain
  that a box is trivial.


Creating Simple Boxes
#####################

|amongoc| comes with shorthands for creating boxes that contain built-in types.
For example `amongoc_box_int` will create a new `amongoc_box` that contains an
:cpp:`int`.


Initializing a Box with a Custom Type
#####################################

Suppose we have a more complex type that we wish to store in a box. To do this,
we just use the same `amongoc_box_init`, passing our custom type as the second
argument:

.. literalinclude:: box.example.cpp
  :start-after: [init-box-aggregate]
  :end-before: end.

In this example, the box we create stores an instance of ``large``, we then pass
a pointer to the box-managed instance to an ``init_large`` function, use the
stored value, and then destory the box.


Initializing a Box with a Custom Type and Destructor Function
#############################################################

It is possible that we need to send a non-trivial boxed value to another
function that doesn't know about what is stored in the box, but that function
may need to destroy the box. If our custom type needs to execute some kind of
destructor, then we will need to associate that destructor with the generated
box. This can be done by passing a destructor function as the third argument
to `amongoc_box_init`:

.. literalinclude:: box.example.cpp
  :start-after: [init-box-dtor]
  :end-before: end.

In this case, ``takes_box`` takes ownership of the box, so we don't call
`amongoc_box_destroy` ourselves. It is the responsibility of ``takes_box`` to
eventually destroy the box or hand it off to someone else. When
`amongoc_box_destroy` is eventually called, a pointer to the stored value will
be passed to the destructor function.


Viewing the Content of a Box
############################

If you have a box and know its contained type, you can access the content of the
box using `amongoc_box_cast`:

.. literalinclude:: box.example.cpp
  :start-after: [box-cast]
  :end-before: end.

The first argument to `amongoc_box_cast` must be a type specifier, and the
second argument must be an :term:`lvalue` of type `amongoc_box`.


Moving from a Box
#################

To steal a value from an `amongoc_box`, use `amongoc_box_take`:

.. literalinclude:: box.example.cpp
  :start-after: [box-take]
  :end-before: end.

The `amongoc_box_take` function expects a modifiable :term:`lvalue` of the box's
contained type as its first argument, and an :term:`lvalue` of `amongoc_box` as
the second. This special function will move the box's contained value into the
target and then release any dynamic storage required in the box, without
executing any destructor function associated with the box.



Using `amongoc::unique_box` in C++
##################################

While `amongoc_box` is perfectly usable from C++ code, it may be more convenient
to use `amongoc::unique_box`, which is a move-only type that will automatically
execute the associated destructor when it is destroyed.


Converting from a C `amongoc_box`
*********************************

The method `amongoc_box::as_unique` is an r-value-qualifed member function on
the C struct type that will return an `amongoc::unique_box` that owns the
associated box:

.. literalinclude:: box.example.cpp
  :start-after: [cxx-box]
  :end-before: end.


Converting back to a C `amongoc_box`
************************************

To relinquish ownership of the box, use
`release() <amongoc::unique_box::release>`:

.. literalinclude:: box.example.cpp
  :start-after: [cxx-release-box]
  :end-before: end.

