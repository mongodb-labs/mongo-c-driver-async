#####################################################
Meta-Reference: Non-Standard Documentation Attributes
#####################################################

Some APIs in this documentation contain non-standard language attributes on
functions, parameters, types, and variables. These only appear in the
documentation and are not part of the actual C API headers. They do not effect
the actual compilation or behavior of the program, and are intended to act as
common shorthands for behavioral guarantees of the annotated APIs.


|attr.transfer|
###############

.. doc-attr:: [[transfer]]

  If a function parameter is annotated with |attr.transfer|, it means that
  passing an object by-value to that function will invalidate the value in the
  caller's scope.

  When an object is passed through a parameter that is marked with
  |attr.transfer|, it becomes the responsibility of the called function to
  manage the lifetime of the associated object. The function **must** eventually
  pass the object to another function that has an |attr.transfer| attribute.\
  [#box-trivial-note]_


Example: POSIX ``close``
************************

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
would be correct to annotate the parameter with |attr.transfer|


Example: C ``free``
*******************

The C function ``free`` could also be annotated as |attr.transfer| on the passed
pointer value::

  void free(void* [[transfer]] p)

This follows similar behavior to ``close()``: Even though the bits of the
pointer are unmodified in the calling scope, the pointer's value itself has
become poisoned and should not be used for any subsequent operation.


Example: `amongoc_box_destroy`
******************************

The `amongoc_box_destroy` function takes an `amongoc_box` by-value, and its parameter
is annotated with |attr.transfer|.

Like with ``close()`` and ``free()``, the caller's copy of the passed value is
unmodifed, but is still invalidated. Like with ``close()`` and ``free()``,
subsequent uses of the object are unspecified/undefined, including attempting to
destroy the box a second time::

  amongoc_box got_box = create_a_box();
  amongoc_box_destroy(got_box);         // Okay
  amongoc_box_destroy(got_box);         // UNDEFINED BEHAVIOR


Dealing with Transferred Objects
********************************

After passing an object to a |attr.transfer| function parameter, the only legal\
[#box-trivial-note]_ operations on that object are reassignment (replacing the
object with a live value) or discarding (letting the object leave scope).


|attr.type|
###########

.. doc-attr:: [[type(T)]]

  Associates a type with a type-erased object. The meaning of this association
  depends on the object container type. The following types are often used with
  |attr.type|:

  - `amongoc_emitter` - Specifies the success result type of the emitter.
  - `amongoc_handler` - Specifies the result type expected by the handler.
  - `amongoc_box` and `amongoc_view` - Specifies the type that is contained
    within the box for use with :c:macro:`amongoc_box_cast`
  - ``void*`` - Specifies the pointed-to type for the pointer.


|attr.storage|
##############

.. doc-attr:: [[storage]]

  When attached to a pointer parameter, this attribute indicates that the
  pointer is treated as uninitialized storage for an object of the appropriate
  type. The API using |attr.storage| will not attempt to destroy or read from
  the pointed-to location.


:doc-attr:`[[optional]]`
########################

.. doc-attr:: [[optional]]

  When applied to a method declaration in a virtual method table, indicates that
  the associated method pointer may be ``NULL``.

  For any methods not declared with :doc-attr:`[[optional]]`, assume that the
  method is required.

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
